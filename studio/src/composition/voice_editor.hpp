// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <string>

#include <QDialog>

class QLineEdit;

class VoiceEditor final : public QDialog
{
	Q_OBJECT

public:
	explicit VoiceEditor(QWidget*);

	void setVoiceName(const std::string&);
	std::string voiceName() const;

private:
	QLineEdit* _nameEdit = nullptr;
};
