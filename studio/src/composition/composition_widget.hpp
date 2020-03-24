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

#include <QWidget>

class QGraphicsView;

namespace aulos
{
	struct CompositionData;
	struct Voice;
}

class CompositionScene;

class CompositionWidget final : public QWidget
{
public:
	CompositionWidget(CompositionScene*, QWidget* parent);

	void addCompositionPart(const std::string& name, const std::shared_ptr<aulos::Voice>&);
	void setComposition(const std::shared_ptr<aulos::CompositionData>&);
	void setInteractive(bool);
	void setPlaybackOffset(double);
	void setVoiceName(const void* id, const std::string& name);

private:
	CompositionScene* const _scene;
	QGraphicsView* _view = nullptr;
	std::shared_ptr<aulos::CompositionData> _composition;
};
