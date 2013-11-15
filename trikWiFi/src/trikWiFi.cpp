/* Copyright 2013 Roman Kurbatov, Yurii Litvinov
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License. */

#include "trikWiFi.h"

#include <QtCore/QStringList>

#include "wpaSupplicantCommunicator.h"

using namespace trikWiFi;

TrikWiFi::TrikWiFi(QString const &interfaceFilePrefix
		, QString const &daemonFile
		, QObject *parent)
	: QObject(parent)
	, mControlInterface(new WpaSupplicantCommunicator(interfaceFilePrefix + "ctrl", daemonFile))
	, mMonitorInterface(new WpaSupplicantCommunicator(interfaceFilePrefix + "mon", daemonFile))
{
	mMonitorInterface->attach();
	int const monitorFileDesc = mMonitorInterface->fileDescriptor();
	if (monitorFileDesc >= 0) {
		mMonitorFileSocketNotifier.reset(new QSocketNotifier(monitorFileDesc, QSocketNotifier::Read));
	}

	QObject::connect(mMonitorFileSocketNotifier.data(), SIGNAL(activated(int)), this, SLOT(receiveMessages()));
}

TrikWiFi::~TrikWiFi()
{
	mMonitorInterface->detach();
}

int TrikWiFi::connect(int id)
{
	QString reply;
	int result = mControlInterface->request("DISCONNECT", reply);
	if (result < 0 || reply != "OK\n") {
		return -1;
	}

	result = mControlInterface->request("SELECT_NETWORK " + QString::number(id), reply);
	if (result < 0 || reply != "OK\n") {
		return -1;
	}

	return 0;
}

int TrikWiFi::disconnect()
{
	QString reply;
	int const result = mControlInterface->request("DISCONNECT", reply);
	if (result == 0 && reply == "OK\n") {
		return 0;
	} else {
		return -1;
	}
}

int TrikWiFi::scan()
{
	QString reply;
	int const result = mControlInterface->request("SCAN", reply);
	if (result == 0 && reply == "OK\n") {
		return 0;
	} else {
		return -1;
	}
}

QList<TrikWiFi::ScanResult> TrikWiFi::scanResults()
{
	int index = 0;
	QList<TrikWiFi::ScanResult> results;

	forever {
		QString const command = "BSS " + QString::number(index++);
		QString reply;

		if (mControlInterface->request(command, reply) < 0) {
			break;
		}

		if (reply.isEmpty() || reply.startsWith("FAIL")) {
			break;
		}

		ScanResult currentResult;
		currentResult.frequency = -1;
		QStringList const lines = reply.split('\n');

		foreach (QString const &line, lines) {
			int const valuePos = line.indexOf('=') + 1;
			if (valuePos < 1) {
				continue;
			}

			if (line.startsWith("ssid=")) {
				currentResult.ssid = line.mid(valuePos);
			} else if (line.startsWith("freq=")) {
				currentResult.frequency = line.mid(valuePos).toInt();
			}
		}

		results.append(currentResult);
	}

	return results;
}

int TrikWiFi::addNetwork()
{
	QString reply;
	mControlInterface->request("ADD_NETWORK", reply);
	if (reply.startsWith("FAIL")) {
		return -1;
	}

	bool ok = false;
	int const id = reply.toInt(&ok);
	if (ok) {
		mControlInterface->request("SET_NETWORK " + QString::number(id) + " disabled 1", reply);
		return id;
	} else {
		return -1;
	}
}

int TrikWiFi::removeNetwork(int id)
{
	QString reply;
	int const result = mControlInterface->request("REMOVE_NETWORK " + QString::number(id), reply);
	if (result == 0 && reply == "OK\n") {
		return 0;
	} else {
		return -1;
	}
}

int TrikWiFi::setSsid(int id, QString const &ssid)
{
	QString reply;
	int const result
			= mControlInterface->request("SET_NETWORK " + QString::number(id) + " ssid \"" + ssid + "\"", reply);

	if (result == 0 && reply == "OK\n") {
		return 0;
	} else {
		return -1;
	}
}

int TrikWiFi::setKey(int id, QString const &key)
{
	QString reply;
	int const result = mControlInterface->request("SET_NETWORK " + QString::number(id) + " psk \"" + key + "\"", reply);
	if (result == 0 && reply == "OK\n") {
		return 0;
	} else {
		return -1;
	}
}

int TrikWiFi::saveConfiguration()
{
	QString reply;
	int const result = mControlInterface->request("SAVE_CONFIG", reply);
	if (result == 0 && reply == "OK\n") {
		return 0;
	} else {
		return -1;
	}
}

QList<TrikWiFi::NetworkConfiguration> TrikWiFi::listNetworks()
{
	QString reply;
	int const result = mControlInterface->request("LIST_NETWORKS", reply);
	if (result < 0 || reply.isEmpty() || reply.startsWith("FAIL")) {
		return QList<NetworkConfiguration>();
	}

	QStringList const lines = reply.split('\n');
	QList<NetworkConfiguration> list;
	foreach (QString const &line, lines) {
		QStringList const values = line.split('\t');
		if (values.size() != 4) {
			continue;
		}

		NetworkConfiguration currentNetwork;
		bool ok = false;
		currentNetwork.id = values[0].toInt(&ok);
		if (!ok) {
			currentNetwork.id = -1;
		}

		currentNetwork.ssid = values[1];
		list.append(currentNetwork);
	}

	return list;
}

void TrikWiFi::processMessage(QString const &message)
{
	if (message.contains("CTRL-EVENT-SCAN-RESULTS")) {
		emit scanFinished();
	}
}

void TrikWiFi::receiveMessages()
{
	while (mMonitorInterface->isPending()) {
		QString message;
		mMonitorInterface->receive(message);
		processMessage(message);
	}
}