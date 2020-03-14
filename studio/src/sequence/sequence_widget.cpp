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

#include "sequence_widget.hpp"

#include "sequence_scene.hpp"

#include <QGraphicsView>
#include <QGridLayout>
#include <QScrollBar>

SequenceWidget::SequenceWidget(SequenceScene* scene, QWidget* parent)
	: QWidget{ parent }
	, _scene{ scene }
{
	const auto layout = new QGridLayout{ this };
	layout->setContentsMargins({});

	_view = new QGraphicsView{ _scene, this };
	_view->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
	layout->addWidget(_view, 0, 0);
}

void SequenceWidget::setInteractive(bool interactive)
{
	_view->setInteractive(interactive);
}

void SequenceWidget::setSequence(const aulos::SequenceData& sequence)
{
	const auto verticalPosition = _scene->setSequence(sequence, _view->size());
	const auto horizontalScrollBar = _view->horizontalScrollBar();
	horizontalScrollBar->setValue(horizontalScrollBar->minimum());
	const auto verticalScrollBar = _view->verticalScrollBar();
	verticalScrollBar->setValue(verticalScrollBar->minimum() + std::lround((verticalScrollBar->maximum() - verticalScrollBar->minimum()) * verticalPosition));
}
