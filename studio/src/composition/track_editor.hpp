// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <QDialog>

class QSpinBox;

class TrackEditor : public QDialog
{
	Q_OBJECT

public:
	explicit TrackEditor(QWidget*);
	~TrackEditor() override;

	void setTrackWeight(unsigned);
	unsigned trackWeight() const;

private:
	QSpinBox* _weightSpin = nullptr;
};
