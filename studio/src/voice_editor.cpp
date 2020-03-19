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
	_nameEdit->setValidator(new QRegExpValidator{ QRegExp{ "\\w*" }, _nameEdit });
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
