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

#include "composition_widget.hpp"

#include "../theme.hpp"
#include "composition_scene.hpp"

#include <aulos/data.hpp>

#include <QGraphicsView>
#include <QGridLayout>
#include <QScrollBar>

CompositionWidget::CompositionWidget(CompositionScene* scene, QWidget* parent)
	: QWidget{ parent }
	, _scene{ scene }
{
	const auto layout = new QGridLayout{ this };
	layout->setContentsMargins({});

	_view = new QGraphicsView{ _scene, this };
	_view->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
	layout->addWidget(_view, 0, 0);
}

void CompositionWidget::addCompositionPart(const std::string& name, const std::shared_ptr<aulos::Voice>& voice)
{
	const auto& part = _composition->_parts.emplace_back(std::make_shared<aulos::PartData>(voice));
	part->_voiceName = name;
	part->_tracks.emplace_back(std::make_shared<aulos::TrackData>(1));
	_scene->appendPart(part);
	_view->horizontalScrollBar()->setValue(_view->horizontalScrollBar()->minimum());
}

void CompositionWidget::setComposition(const std::shared_ptr<aulos::CompositionData>& composition)
{
	_scene->reset(composition, _view->width());
	_view->horizontalScrollBar()->setValue(_view->horizontalScrollBar()->minimum());
	_composition = composition;
}

void CompositionWidget::setInteractive(bool interactive)
{
	_view->setInteractive(interactive);
}

void CompositionWidget::setPlaybackOffset(double step)
{
	const auto sceneCursorRect = _scene->setCurrentStep(_scene->startOffset() + step);
	QRect viewCursorRect{ _view->mapFromScene(sceneCursorRect.topLeft()), _view->mapFromScene(sceneCursorRect.bottomRight()) };
	const auto viewportRect = _view->viewport()->rect();
	if (viewCursorRect.right() > viewportRect.right() - kCompositionPageSwitchMargin)
		viewCursorRect.moveRight(viewCursorRect.right() + viewportRect.width() - kCompositionPageSwitchMargin);
	else if (viewCursorRect.right() < viewportRect.left())
		viewCursorRect.moveRight(viewCursorRect.right() - viewportRect.width() / 2);
	else
		return;
	_view->ensureVisible({ _view->mapToScene(viewCursorRect.topLeft()), _view->mapToScene(viewCursorRect.bottomRight()) }, 0);
}

void CompositionWidget::setVoiceName(const void* id, const std::string& name)
{
	_scene->updateVoice(id, name);
	_view->horizontalScrollBar()->setValue(_view->horizontalScrollBar()->minimum());
}
