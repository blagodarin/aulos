//
// This file is part of the Aulos toolkit.
// Copyright (C) 2020 Sergei Blagodarin.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include "track_editor.hpp"

#include <QDialogButtonBox>
#include <QGridLayout>
#include <QLabel>
#include <QSpinBox>

TrackEditor::TrackEditor(QWidget* parent)
	: QDialog{ parent, Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint }
{
	setWindowTitle(tr("Track Editor"));

	const auto rootLayout = new QGridLayout{ this };

	const auto weightLabel = new QLabel{ tr("Track &weight:"), this };
	rootLayout->addWidget(weightLabel, 0, 0);

	_weightSpin = new QSpinBox{ this };
	_weightSpin->setRange(1, 255);
	rootLayout->addWidget(_weightSpin, 0, 1);
	weightLabel->setBuddy(_weightSpin);

	rootLayout->addItem(new QSpacerItem{ 0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding }, 1, 0, 1, 2);

	const auto buttonBox = new QDialogButtonBox{ QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this };
	rootLayout->addWidget(buttonBox, 2, 0, 1, 2);
	connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

TrackEditor::~TrackEditor() = default;

void TrackEditor::setTrackWeight(unsigned weight)
{
	_weightSpin->setValue(weight);
}

unsigned TrackEditor::trackWeight() const
{
	return _weightSpin->value();
}
