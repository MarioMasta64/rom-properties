/***************************************************************************
 * ROM Properties Page shell extension. (KDE4/KF5)                         *
 * MessageSound.hpp: Message sound effects abstraction class.              *
 *                                                                         *
 * Copyright (c) 2018-2020 by David Korth.                                 *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "stdafx.h"
#include "MessageSound.hpp"

#include <QtCore/QPluginLoader>
#include <QtCore/QVariant>

#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
#  include <kmessageboxnotifyinterface.h>
#else
#  include <knotification.h>
#endif

/**
 * Play a message sound effect.
 * @param notificationType Notification type.
 * @param message Message for logging. (not supported on all systems)
 * @param parent Parent window. (not supported on all systems)
 */
void MessageSound::play(QMessageBox::Icon notificationType, const QString &message, QWidget *parent)
{
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
	QPluginLoader lib(QStringLiteral("kf5/FrameworkIntegrationPlugin"));
	QObject *const rootObj = lib.instance();
	if (rootObj) {
		KMessageBoxNotifyInterface *iface = rootObj->property(KMESSAGEBOXNOTIFY_PROPERTY)
			.value<KMessageBoxNotifyInterface*>();
		if (iface) {
			iface->sendNotification(notificationType, message, parent);
		}
	}
#else /* QT_VERSION < QT_VERSION_CHECK(5,0,0) */
	// FIXME: KNotification::event() doesn't seem to work.
	// This might not be too important nowadays, since KDE4 is ancient...
	QString messageType;
	switch (notificationType) {
		default:
		case QMessageBox::Information:
			messageType = QLatin1String("messageInformation");
			break;
		case QMessageBox::Warning:
			messageType = QLatin1String("messageWarning");
			break;
		case QMessageBox::Question:
			messageType = QLatin1String("messageQuestion");
			break;
		case QMessageBox::Critical:
			messageType = QLatin1String("messageCritical");
			break;
	}
	KNotification::event(messageType, message, QPixmap(), parent);
#endif
}
