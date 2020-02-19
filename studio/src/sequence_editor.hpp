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

#include <memory>

#include <QDialog>

class QGraphicsView;
class QLabel;

namespace aulos
{
	struct SequenceData;
	struct Voice;
}

class Player;
class SequenceScene;

class SequenceEditor : public QDialog
{
	Q_OBJECT

public:
	SequenceEditor(QWidget*);
	~SequenceEditor() override;

	void setSequence(const aulos::Voice&, const aulos::SequenceData&);
	aulos::SequenceData sequence() const;

private:
	const std::unique_ptr<aulos::Voice> _voice;
	SequenceScene* const _scene;
	QGraphicsView* _sequenceView;
	std::unique_ptr<Player> _player;
};
