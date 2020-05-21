// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <QDialog>

class QLineEdit;

class InfoEditor : public QDialog
{
	Q_OBJECT

public:
	explicit InfoEditor(QWidget*);
	~InfoEditor() override;

	void setCompositionAuthor(const QString&);
	void setCompositionTitle(const QString&);
	QString compositionAuthor() const;
	QString compositionTitle() const;

private:
	QLineEdit* _titleEdit = nullptr;
	QLineEdit* _authorEdit = nullptr;
};
