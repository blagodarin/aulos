// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#include "voice_editor.hpp"

#include <QDialogButtonBox>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QValidator>

VoiceEditor::VoiceEditor(QWidget* parent)
	: QDialog{ parent, Qt::WindowTitleHint | Qt::CustomizeWindowHint | Qt::WindowCloseButtonHint }
{
	setWindowTitle(tr("Voice Editor"));

	const auto rootLayout = new QGridLayout{ this };

	const auto nameLabel = new QLabel{ tr("Voice &name:"), this };
	rootLayout->addWidget(nameLabel, 0, 0);

	_nameEdit = new QLineEdit{ this };
	_nameEdit->setMaxLength(64);
	_nameEdit->setValidator(new QRegularExpressionValidator{ QRegularExpression{ "\\w*" }, _nameEdit });
	rootLayout->addWidget(_nameEdit, 0, 1);
	nameLabel->setBuddy(_nameEdit);

	const auto buttonBox = new QDialogButtonBox{ QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this };
	rootLayout->addWidget(buttonBox, 1, 0, 1, 2);
	connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void VoiceEditor::setVoiceName(const std::string& name)
{
	_nameEdit->setText(QString::fromStdString(name));
}

std::string VoiceEditor::voiceName() const
{
	return _nameEdit->text().toStdString();
}
