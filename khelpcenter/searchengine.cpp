#include "stdlib.h"

#include <kapplication.h>
#include <kconfig.h>
#include <kdebug.h>
#include <kstandarddirs.h>
#include <kprocess.h>
#include <klocale.h>
#include <kmessagebox.h>

#include "docmetainfo.h"
#include "formatter.h"
#include "view.h"
#include "searchhandler.h"

#include "searchengine.h"

namespace KHC
{

SearchTraverser::SearchTraverser( SearchEngine *engine, int level ) :
  mEngine( engine), mLevel( level ), mEntry( 0 )
{
#if 0
  kdDebug() << "SearchTraverser(): " << mLevel << endl;

  kdDebug() << "  0x" << QString::number( int( this ), 16 ) << endl;
#endif
}

SearchTraverser::~SearchTraverser()
{
#if 0
  if ( mEntry ) {
    kdDebug() << "~SearchTraverser(): " << mLevel << " " << mEntry->name() << endl;
  } else {
    kdDebug() << "~SearchTraverser(): null entry" << endl;
  }

  kdDebug() << "  0x" << QString::number( int( this ), 16 ) << endl;
#endif

  QString section;
  if ( parentEntry() ) {
    section = parentEntry()->name();
  } else {
    section = ("Unknown Section");
  }

  if ( !mResult.isEmpty() ) {
    mEngine->view()->writeSearchResult(
      mEngine->formatter()->sectionHeader( section ) );
    mEngine->view()->writeSearchResult( mResult );
  }
}

void SearchTraverser::process( DocEntry * )
{
  kdDebug() << "SearchTraverser::process()" << endl;
}

void SearchTraverser::startProcess( DocEntry *entry )
{
//  kdDebug() << "SearchTraverser::startProcess(): " << entry->name() << "  "
//    << "SEARCH: '" << entry->search() << "'" << endl;

  mEntry = entry;

  if ( !entry->isSearchable() || !entry->searchEnabled() ) {
    mNotifyee->endProcess( entry, this );
    return;
  }

  kdDebug() << "SearchTraverser::startProcess(): " << entry->identifier()
    << endl;

  SearchHandler *handler = mEngine->handler( entry->documentType() );

  if ( !handler ) {
    QString txt;
    if ( entry->documentType().isEmpty() ) {
      txt = i18n("Error: No document type specified.");
    } else { 
      txt = i18n("Error: No search handler for document type '%1'.")
        .arg( entry->documentType() );
    }
    showSearchError( txt );
    return;
  }

  connect( handler, SIGNAL( searchFinished( const QString & ) ),
    SLOT( showSearchResult( const QString & ) ) );

  handler->search( entry->identifier(), mEngine->words(), mEngine->maxResults(),
    mEngine->operation() );

  kdDebug() << "SearchTraverser::startProcess() done: " << entry->name() << endl;
}

DocEntryTraverser *SearchTraverser::createChild( DocEntry *parentEntry )
{
  kdDebug() << "SearchTraverser::createChild() level " << mLevel << endl;

  if ( mLevel >= 3 ) {
    ++mLevel;
    return this;
  } else {
    DocEntryTraverser *t = new SearchTraverser( mEngine, mLevel + 1 );
    t->setParentEntry( parentEntry );
    return t;
  }
}

DocEntryTraverser *SearchTraverser::parentTraverser()
{
//  kdDebug() << "SearchTraverser::parentTraverser(): level: " << mLevel << endl;

  if ( mLevel > 3 ) {
    return this;
  } else {
    return mParent;
  }
}

void SearchTraverser::deleteTraverser()
{
  kdDebug() << "SearchTraverser::deleteTraverser()" << endl;

  if ( mLevel > 3 ) {
    --mLevel;
  } else {
    delete this;
  }
}

void SearchTraverser::showSearchError( const QString &error )
{
  mResult += mEngine->formatter()->docTitle( mEntry->name() );
  mResult += mEngine->formatter()->paragraph( error );

  mNotifyee->endProcess( mEntry, this );
}

void SearchTraverser::showSearchResult( const QString &result )
{
  kdDebug() << "SearchTraverser::showSearchResult(): " << mEntry->name()
    << endl;

  mResult += mEngine->formatter()->docTitle( mEntry->name() );
  mResult += mEngine->formatter()->processResult( result );

  mNotifyee->endProcess( mEntry, this );
}

void SearchTraverser::finishTraversal()
{
  mEngine->view()->writeSearchResult( mEngine->formatter()->footer() );
  mEngine->view()->endSearchResult();

  mEngine->finishSearch();
}


SearchEngine::SearchEngine( View *destination )
  : QObject(),
    mProc( 0 ), mSearchRunning( false ), mView( destination ),
    mRootTraverser( 0 )
{
  mLang = KGlobal::locale()->language().left( 2 );
}

SearchEngine::~SearchEngine()
{
  delete mRootTraverser;
}

bool SearchEngine::initSearchHandlers()
{
  QStringList resources = KGlobal::dirs()->findAllResources(
    "appdata", "searchhandlers/*.desktop" );
  QStringList::ConstIterator it;
  for( it = resources.begin(); it != resources.end(); ++it ) {
    QString filename = *it;
    kdDebug() << "SearchEngine::initSearchHandlers(): " << filename << endl;
    SearchHandler *handler = SearchHandler::initFromFile( filename );
    if ( !handler ) {
      KMessageBox::sorry( mView->widget(),
        i18n("Unable to initialize SearchHandler from file '%1'.")
        .arg( filename ) );
    } else {
      QStringList documentTypes = handler->documentTypes();
      QStringList::ConstIterator it;
      for( it = documentTypes.begin(); it != documentTypes.end(); ++it ) {
        mHandlers.insert( *it, handler );
      }
    }
  }
  
  if ( mHandlers.isEmpty() ) {
    KMessageBox::sorry( mView->widget(),
      i18n("No valid search handler found.") );
    return false;
  }

  return true;
}

void SearchEngine::searchStdout(KProcess *, char *buffer, int len)
{
  if ( !buffer || len == 0 )
    return;

  QString bufferStr;
  char *p;
  p = (char*) malloc( sizeof(char) * (len+1) );
  p = strncpy( p, buffer, len );
  p[len] = '\0';

  mSearchResult += bufferStr.fromUtf8(p);

  free(p);
}

void SearchEngine::searchStderr(KProcess *, char *buffer, int len)
{
  if ( !buffer || len == 0 )
    return;

  mStderr.append( QString::fromUtf8( buffer, len ) );
}

void SearchEngine::searchExited(KProcess *)
{
  kdDebug() << "Search terminated" << endl;
  mSearchRunning = false;
}

bool SearchEngine::search( QString words, QString method, int matches,
                           QString scope )
{
  if ( mSearchRunning ) return false;

  // These should be removed
  mWords = words;
  mMethod = method;
  mMatches = matches;
  mScope = scope;

  // Saner variables to store search parameters:
  mWordList = QStringList::split( " ", words );
  mMaxResults = matches;
  if ( method == "or" ) mOperation = Or;
  else mOperation = And;

  KConfig *cfg = KGlobal::config();
  cfg->setGroup( "Search" );
  QString commonSearchProgram = cfg->readPathEntry( "CommonProgram" );
  bool useCommon = cfg->readBoolEntry( "UseCommonProgram", false );
  
  if ( commonSearchProgram.isEmpty() || !useCommon ) {
    if ( !mView ) {
      return false;
    }

    mView->beginSearchResult();
    mView->writeSearchResult( formatter()->header( i18n("Search Results") ) );
    mView->writeSearchResult( formatter()->title(
      i18n("Search Results for '%1':").arg( words ) ) );

    if ( mRootTraverser ) {
      kdDebug() << "SearchEngine::search(): mRootTraverser not null." << endl;
      return false;
    }
    mRootTraverser = new SearchTraverser( this, 0 );
    DocMetaInfo::self()->startTraverseEntries( mRootTraverser );

    return true;
  } else {
    QString lang = KGlobal::locale()->language().left(2);

    if ( lang.lower() == "c" || lang.lower() == "posix" )
	  lang = "en";

    // if the string contains '&' replace with a '+' and set search method to and
    if (mWords.find("&") != -1) {
      mWords.replace("&", " ");
      method = "and";
    }
 
    // replace whitespace with a '+'
    mWords = mWords.stripWhiteSpace();
    mWords = mWords.simplifyWhiteSpace();
    mWords.replace(QRegExp("\\s"), "+");
    
    commonSearchProgram = substituteSearchQuery( commonSearchProgram );

    kdDebug() << "Common Search: " << commonSearchProgram << endl;

    mProc = new KProcess();

    QStringList cmd = QStringList::split( " ", commonSearchProgram );
    QStringList::ConstIterator it;
    for( it = cmd.begin(); it != cmd.end(); ++it ) {
      QString arg = *it;
      if ( arg.left( 1 ) == "\"" && arg.right( 1 ) =="\"" ) {
        arg = arg.mid( 1, arg.length() - 2 );
      }
      *mProc << arg.utf8();
    }

    connect( mProc, SIGNAL( receivedStdout( KProcess *, char *, int ) ),
             SLOT( searchStdout( KProcess *, char *, int ) ) );
    connect( mProc, SIGNAL( receivedStderr( KProcess *, char *, int ) ),
             SLOT( searchStderr( KProcess *, char *, int ) ) );
    connect( mProc, SIGNAL( processExited( KProcess * ) ),
             SLOT( searchExited( KProcess * ) ) );

    mSearchRunning = true;
    mSearchResult = "";
    mStderr = "<b>" + commonSearchProgram + "</b>\n\n";

    mProc->start(KProcess::NotifyOnExit, KProcess::All);

    while (mSearchRunning && mProc->isRunning())
      kapp->processEvents();

    if ( !mProc->normalExit() || mProc->exitStatus() != 0 ) {
      kdError() << "Unable to run search program '" << commonSearchProgram
                << "'" << endl;
      delete mProc;
      
      return false;
    }

    delete mProc;

    // modify the search result
    mSearchResult = mSearchResult.replace("http://localhost/", "file:/");
    mSearchResult = mSearchResult.mid( mSearchResult.find( '<' ) );

    mView->beginSearchResult();
    mView->writeSearchResult( mSearchResult );
    mView->endSearchResult();

    emit searchFinished();
  }

  return true;
}

QString SearchEngine::substituteSearchQuery( const QString &query )
{
  QString result = query;
  result.replace( "%k", mWords );
  result.replace( "%n", QString::number( mMatches ) );
  result.replace( "%m", mMethod );
  result.replace( "%l", mLang );
  result.replace( "%s", mScope );

  return result;
}

QString SearchEngine::substituteSearchQuery( const QString &query,
  const QString &identifier, const QStringList &words, int maxResults,
  Operation operation )
{
  QString result = query;
  result.replace( "%i", identifier );
  result.replace( "%w", words.join( "+" ) );
  result.replace( "%m", QString::number( maxResults ) );
  QString o;
  if ( operation == Or ) o = "or";
  else o = "and";
  result.replace( "%o", o );
// FIXME: handle lang
//  result.replace( "%l", mLang );

  return result;
}

Formatter *SearchEngine::formatter() const
{
  return mView->formatter();
}

View *SearchEngine::view() const
{
  return mView;
}

void SearchEngine::finishSearch()
{
  delete mRootTraverser;
  mRootTraverser = 0;

  emit searchFinished();  
}

QString SearchEngine::errorLog() const
{
  return mStderr;
}

bool SearchEngine::isRunning() const
{
  return mSearchRunning;
}

SearchHandler *SearchEngine::handler( const QString &documentType ) const
{
  QMap<QString,SearchHandler *>::ConstIterator it;
  it = mHandlers.find( documentType );

  if ( it == mHandlers.end() ) return 0;
  else return *it;
}

QStringList SearchEngine::words() const
{
  return mWordList;
}

int SearchEngine::maxResults() const
{
  return mMaxResults;
}

SearchEngine::Operation SearchEngine::operation() const
{
  return mOperation;
}

}

#include "searchengine.moc"

// vim:ts=2:sw=2:et
