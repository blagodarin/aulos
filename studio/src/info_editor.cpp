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

#include "info_editor.hpp"

#include <QDialogButtonBox>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QValidator>

InfoEditor::InfoEditor(QWidget* parent)
	: QDialog{ parent, Qt::WindowTitleHint | Qt::CustomizeWindowHint | Qt::WindowCloseButtonHint }
{
	setWindowTitle(tr("Composition Information"));

	const auto rootLayout = new QGridLayout{ this };

	const auto validator = new QRegExpValidator{ QRegExp{ "[^\"]*" }, this };

	const auto titleLabel = new QLabel{ tr("&Title:"), this };
	rootLayout->addWidget(titleLabel, 0, 0);

	_titleEdit = new QLineEdit{ this };
	_titleEdit->setValidator(validator);
	rootLayout->addWidget(_titleEdit, 0, 1);
	titleLabel->setBuddy(_titleEdit);

	const auto authorLabel = new QLabel{ tr("&Author:"), this };
	rootLayout->addWidget(authorLabel, 1, 0);

	_authorEdit = new QLineEdit{ this };
	_authorEdit->setValidator(validator);
	rootLayout->addWidget(_authorEdit, 1, 1);
	authorLabel->setBuddy(_authorEdit);

	rootLayout->addItem(new QSpacerItem{ 0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding }, 2, 0, 1, 2);

	const auto buttonBox = new QDialogButtonBox{ QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this };
	rootLayout->addWidget(buttonBox, 3, 0, 1, 2);
	connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

InfoEditor::~InfoEditor() = default;

void InfoEditor::setCompositionAuthor(const QString& author)
{
	_authorEdit->setText(author);
}

void InfoEditor::setCompositionTitle(const QString& title)
{
	_titleEdit->setText(title);
}

QString InfoEditor::compositionAuthor() const
{
	return _authorEdit->text();
}

QString InfoEditor::compositionTitle() const
{
	return _titleEdit->text();
}
