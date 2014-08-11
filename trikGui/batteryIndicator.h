/* Copyright 2014 CyberTech Labs Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License. */

#pragma once

#include <QtCore/qglobal.h>
#include <QtCore/QTimer>

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
	#include <QtGui/QWidget>
	#include <QtGui/QVBoxLayout>
	#include <QtGui/QLabel>
#else
	#include <QtWidgets/QWidget>
	#include <QtWidgets/QVBoxLayout>
	#include <QtWidgets/QLabel>
#endif

#include <trikControl/brick.h>

namespace trikGui {

class BatteryIndicator : public QWidget
{
	Q_OBJECT
public:
	explicit BatteryIndicator(trikControl::Brick &brick, QWidget *parent = 0);

private:
	trikControl::Brick &mBrick;
	QVBoxLayout mLayout;
	QLabel mVoltage;
	QTimer mRenewTimer;
	int const mRenewInterval = 1000;

public slots:
	void renew();
};

}
