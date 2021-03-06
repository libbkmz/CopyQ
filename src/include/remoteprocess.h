/*
    Copyright (c) 2013, Lukas Holecek <hluk@email.cz>

    This file is part of CopyQ.

    CopyQ is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    CopyQ is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with CopyQ.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef REMOTEPROCESS_H
#define REMOTEPROCESS_H

#include <QObject>
#include <QProcess>

class QByteArray;
class QLocalServer;
class QLocalSocket;
class QString;

/**
 * Starts process and handles communication with it.
 */
class RemoteProcess : public QObject
{
    Q_OBJECT
public:
    explicit RemoteProcess(QObject *parent = NULL);

    ~RemoteProcess();

    /**
     * Starts server and executes current binary (argv[0]) with given @a arguments.
     */
    void start(const QString &newServerName, const QStringList &arguments);

    /**
     * Send message to remote process.
     */
    bool writeMessage(const QByteArray &msg);

    /**
     * Return true only if both server and process are started.
     */
    bool isConnected() const;

    void closeConnection();

    QProcess &process() { return m_process; }

signals:
    /**
     * Remote processed sends @a message.
     */
    void newMessage(const QByteArray &message);

    /**
     * An error occurred with connection.
     */
    void connectionError();

private slots:
    void readyRead();

private:
    QProcess m_process;
    QLocalServer *m_server;
    QLocalSocket *m_socket;
};

#endif // REMOTEPROCESS_H
