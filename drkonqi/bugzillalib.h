/*******************************************************************
* bugzillalib.h
* Copyright  2009, 2011   Dario Andres Rodriguez <andresbajotierra@gmail.com>
* Copyright  2012  George Kiagiadakis <kiagiadakis.george@gmail.com>
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License as
* published by the Free Software Foundation; either version 2 of
* the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
******************************************************************/

#ifndef BUGZILLALIB__H
#define BUGZILLALIB__H

#include <kxmlrpcclient/client.h>

#include <QtCore/QObject>
#include <QtCore/QMap>
#include <QtCore/QStringList>

#include <QtXml/QDomDocument>

namespace KIO { class Job; }
class KJob;
class QString;
class QByteArray;

//Typedefs for Bug Report Listing
typedef QMap<QString, QString>   BugMap; //Report basic fields map
typedef QList<BugMap>           BugMapList; //List of reports

//Main bug report data, full fields + comments
class BugReport
{
public:
    enum Status {
        UnknownStatus,
        Unconfirmed,
        New,
        Assigned,
        Reopened,
        Resolved,
        NeedsInfo,
        Verified,
        Closed
    };

    enum Resolution {
        UnknownResolution,
        NotResolved,
        Fixed,
        Invalid,
        WontFix,
        Later,
        Remind,
        Duplicate,
        WorksForMe,
        Moved,
        Upstream,
        Downstream,
        WaitingForInfo,
        Backtrace,
        Unmaintained
    };

    BugReport()
      : m_isValid(false),
        m_status(UnknownStatus),
        m_resolution(UnknownResolution)
    {}

    void setBugNumber(const QString & value) {
        setData("bug_id", value);
    }
    QString bugNumber() const {
        return getData("bug_id");
    }
    int bugNumberAsInt() const {
        return getData("bug_id").toInt();
    }

    void setShortDescription(const QString & value) {
        setData("short_desc", value);
    }
    QString shortDescription() const {
        return getData("short_desc");
    }

    void setProduct(const QString & value) {
        setData("product", value);
    }
    QString product() const {
        return getData("product");
    }

    void setComponent(const QString & value) {
        setData("component", value);
    }
    QString component() const {
        return getData("component");
    }

    void setVersion(const QString & value) {
        setData("version", value);
    }
    QString version() const {
        return getData("version");
    }

    void setOperatingSystem(const QString & value) {
        setData("op_sys", value);
    }
    QString operatingSystem() const {
        return getData("op_sys");
    }

    void setPlatform(const QString & value) {
        setData("rep_platform", value);
    }
    QString platform() const {
        return getData("rep_platform");
    }

    void setBugStatus(const QString &status);
    QString bugStatus() const {
        return getData("bug_status");
    }

    void setResolution(const QString &resolution);
    QString resolution() const {
        return getData("resolution");
    }

    Status statusValue() const {
        return m_status;
    }

    Resolution resolutionValue() const {
        return m_resolution;
    }

    void setPriority(const QString & value) {
        setData("priority", value);
    }
    QString priority() const {
        return getData("priority");
    }

    void setBugSeverity(const QString & value) {
        setData("bug_severity", value);
    }
    QString bugSeverity() const {
        return getData("bug_severity");
    }

    void setDescription(const QString & desc) {
        m_commentList.insert(0, desc);
    }
    QString description() const {
        return m_commentList.at(0);
    }

    void setComments(const QStringList & comm) {
        m_commentList.append(comm);
    }
    QStringList comments() const {
        return m_commentList.mid(1);
    }

    void setMarkedAsDuplicateOf(const QString & dupID) {
        setData("dup_id", dupID);
    }
    QString markedAsDuplicateOf() const {
        return getData("dup_id");
    }

    void setVersionFixedIn(const QString & dupID) {
        setData("cf_versionfixedin", dupID);
    }
    QString versionFixedIn() const {
        return getData("cf_versionfixedin");
    }
    
    void setValid(bool valid) {
        m_isValid = valid;
    }
    bool isValid() const {
        return m_isValid;
    }

    /**
     * @return true if the bug report is still open
     * @note false does not mean, that the bug report is closed,
     * as the status could be unknown
     */
    bool isOpen() const {
        return isOpen(m_status);
    }

    static bool isOpen(Status status) {
        return (status == Unconfirmed || status == New || status == Assigned || status == Reopened);
    }

    /**
     * @return true if the bug report is closed
     * @note false does not mean, that the bug report is still open,
     * as the status could be unknown
     */
    bool isClosed() const {
        return isClosed(m_status);
    }

    static bool isClosed(Status status) {
        return (status == Resolved || status == NeedsInfo || status == Verified || status == Closed);
    }

    static Status parseStatus(const QString &text);
    static Resolution parseResolution(const QString &text);

private:
    void setData(const QString & key, const QString & val) {
        m_dataMap.insert(key, val);
    }
    QString getData(const QString & key) const {
        return m_dataMap.value(key);
    }

    bool        m_isValid;
    Status      m_status;
    Resolution  m_resolution;

    BugMap      m_dataMap;
    QStringList m_commentList;
};

//XML parser that creates a BugReport object
class BugReportXMLParser
{
public:
    BugReportXMLParser(const QByteArray &);

    BugReport parse();

    bool isValid() const {
        return m_valid;
    }

private:
    QString getSimpleValue(const QString &);

    bool            m_valid;
    QDomDocument    m_xml;
};

class BugListCSVParser
{
public:
    BugListCSVParser(const QByteArray&);

    bool isValid() const {
        return m_isValid;
    }

    BugMapList parse();

private:
    bool m_isValid;
    QByteArray  m_data;
};

class BugzillaManager : public QObject
{
    Q_OBJECT

public:
    BugzillaManager(QObject *parent = 0);

    /* Login methods */
    void tryLogin();
    bool getLogged() const;

    void setLoginData(const QString &, const QString &);
    QString getUsername() const;

    /* Bugzilla Action methods */
    void fetchBugReport(int, QObject * jobOwner = 0);

    void searchBugs(const QStringList & products, const QString & severity,
                    const QString & date_start, const QString & date_end , QString comment);
    
    void sendReport(const BugReport & report);
    
    void attachTextToReport(const QString & text, const QString & filename,
                            const QString & description, int bugId, const QString & comment);

    void addMeToCC(int bugId);

    void checkVersionsForProduct(const QString &);
    
    /* Misc methods */
    QString urlForBug(int bug_number) const;

    void setCustomBugtrackerUrl(const QString &);

    void stopCurrentSearch();
    
private Q_SLOTS:
    /* Slots to handle KJob::finished */
    void fetchBugJobFinished(KJob*);
    void searchBugsJobFinished(KJob*);
    void checkVersionJobFinished(KJob*);

    void callMessage(const QList<QVariant> & result, const QVariant & id);
    void callFault(int errorCode, const QString & errorString, const QVariant &id);

Q_SIGNALS:
    /* Bugzilla actions finished successfully */
    void loginFinished(bool);
    void bugReportFetched(BugReport, QObject *);
    void searchFinished(const BugMapList &);
    void reportSent(int);
    void attachToReportSent(int, int);
    void addMeToCCFinished(int);
    void checkVersionsForProductFinished(const QStringList);

    /* Bugzilla actions had errors */
    void loginError(const QString & errorMsg, const QString & extendedErrorMsg = QString());
    void bugReportError(const QString &, QObject *);
    void searchError(const QString &);
    void sendReportError(const QString & errorMsg, const QString & extendedErrorMsg = QString());
    void sendReportErrorInvalidValues(); //To use default values
    void attachToReportError(const QString & errorMsg, const QString & extendedErrorMsg = QString());
    void addMeToCCError(const QString & errorMsg, const QString & extendedErrorMsg = QString());
    void checkVersionsForProductError();

private:
    QString     m_username;
    QString     m_password;
    bool        m_logged;

    KIO::Job *  m_searchJob;
    KXmlRpc::Client *m_xmlRpcClient;

    QString     m_bugTrackerUrl;
};

#endif
