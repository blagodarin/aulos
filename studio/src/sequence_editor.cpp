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

#include "sequence_editor.hpp"

#include "utils.hpp"

#include <aulos/data.hpp>

#include <QDialogButtonBox>
#include <QGridLayout>
#include <QLabel>

SequenceEditor::SequenceEditor(QWidget* parent)
	: QDialog{ parent, Qt::WindowTitleHint | Qt::CustomizeWindowHint | Qt::WindowCloseButtonHint }
	, _sequence{ std::make_unique<aulos::SequenceData>() }
{
	setWindowTitle(tr("Sequence Editor"));

	const auto rootLayout = new QGridLayout{ this };

	_sequenceLabel = new QLabel{ this };
	rootLayout->addWidget(_sequenceLabel, 0, 0);

	rootLayout->addItem(new QSpacerItem{ 0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding }, 1, 0);

	const auto buttonBox = new QDialogButtonBox{ QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this };
	rootLayout->addWidget(buttonBox, 2, 0);
	connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

SequenceEditor::~SequenceEditor() = default;

void SequenceEditor::setSequence(const aulos::SequenceData& sequence)
{
	*_sequence = sequence;
	_sequenceLabel->setText(::makeSequenceName(sequence, true));
}

aulos::SequenceData SequenceEditor::sequence() const
{
	return *_sequence;
}
