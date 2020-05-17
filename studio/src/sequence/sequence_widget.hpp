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

#pragma once

#include <aulos/data.hpp>

#include <QWidget>

class QGraphicsView;

namespace aulos
{
	struct SequenceData;
}

class SequenceScene;

class SequenceWidget final : public QWidget
{
	Q_OBJECT

public:
	explicit SequenceWidget(QWidget* parent);

	void setInteractive(bool);
	void setSequence(const std::shared_ptr<aulos::SequenceData>&);
	std::shared_ptr<aulos::SequenceData> sequence() const { return _sequenceData; }

signals:
	void noteActivated(aulos::Note);
	void sequenceChanged();

private:
	SequenceScene* const _scene;
	QGraphicsView* _view = nullptr;
	std::shared_ptr<aulos::SequenceData> _sequenceData;
};
