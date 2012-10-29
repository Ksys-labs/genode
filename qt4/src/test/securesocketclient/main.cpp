/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Nokia Corporation and its Subsidiary(-ies) nor
**     the names of its contributors may be used to endorse or promote
**     products derived from this software without specific prior written
**     permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QApplication>
#include <QMessageBox>

#include <QSslConfiguration>
#include <QSslSocket>
#include <QSslCipher>
#include <QList>

#include <QDir>
#include <QFile>

#include <QDebug>

#include "sslclient.h"

void scanDir(QDir dir)
{
	QFileInfoList entry = dir.entryInfoList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot);
	foreach(QFileInfo fi, entry)
	{
		qDebug() << fi.filePath();
		if (fi.isDir())
			scanDir(fi.filePath());
	}
}

int main(int argc, char **argv)
{
    Q_INIT_RESOURCE(securesocketclient);

    QApplication app(argc, argv);

    if (!QSslSocket::supportsSsl()) {
	QMessageBox::information(0, "Secure Socket Client",
				 "This system does not support OpenSSL.");
        return -1;
    }

	QSslConfiguration sslConfig = QSslConfiguration::defaultConfiguration();
	sslConfig.setProtocol( QSsl::TlsV1 );
	QSslConfiguration::setDefaultConfiguration( sslConfig );
	Q_ASSERT(QSslConfiguration::defaultConfiguration().protocol() == QSsl::TlsV1);

	QList<QSslCipher> lst = QSslSocket::supportedCiphers();

	foreach(const QSslCipher& c, lst)
	{
		qDebug() << c.name();
	}

    SslClient client;
    client.show();

    return app.exec();
}
