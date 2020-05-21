// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#include "track_editor.hpp"

#include <QDialogButtonBox>
#include <QGridLayout>
#include <QLabel>
#include <QSpinBox>

TrackEditor::TrackEditor(QWidget* parent)
	: QDialog{ parent, Qt::WindowTitleHint | Qt::CustomizeWindowHint | Qt::WindowCloseButtonHint }
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
