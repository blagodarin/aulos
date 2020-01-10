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

#include "composition_scene.hpp"

#include <aulos.hpp>

#include <array>
#include <numeric>
#include <vector>

#include <QGraphicsRectItem>
#include <QPainter>

namespace
{
	constexpr auto kScaleX = 10.0;
	constexpr auto kScaleY = 40.0;

	const QColor kBackgroundColor{ "#444" };
	const std::array<QColor, 2> kTrackBackgroundColor{ "#999", "#888" };
	const std::array<QColor, 5> kFragmentBackgroundColor{ "#f88", "#ff8", "#8f8", "#8ff", "#88f" };
	const std::array<QColor, 5> kFragmentFrameColor{ "#800", "#880", "#080", "#088", "#008" };
	const QColor kCursorColor{ "#000" };

	class FragmentItem : public QGraphicsItem
	{
	public:
		FragmentItem(size_t sequenceIndex, size_t x, size_t y, size_t duration, QGraphicsItem* parent = nullptr)
			: QGraphicsItem{ parent }
			, _sequenceIndex{ QString::number(sequenceIndex + 1) }
			, _rect{ x * kScaleX, y * kScaleY, duration * kScaleX, kScaleY }
		{}

		QRectF boundingRect() const override { return _rect; }
		void setBrush(const QBrush& brush) { _brush = brush; }
		void setPen(const QPen& pen) { _pen = pen; }

		void paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) override
		{
			painter->setBrush(_brush);
			painter->setPen(_pen);
			const auto right = _rect.right() - kScaleX / 2;
			const std::array<QPointF, 5> points{
				QPointF{ _rect.left(), _rect.top() },
				QPointF{ right, _rect.top() },
				QPointF{ _rect.right(), _rect.center().y() },
				QPointF{ right, _rect.bottom() },
				QPointF{ _rect.left(), _rect.bottom() },
			};
			painter->drawConvexPolygon(points.data(), static_cast<int>(points.size()));
			auto font = painter->font();
			font.setPixelSize(kScaleY / 2);
			painter->setFont(font);
			painter->drawText(QRectF{ QPointF{ _rect.left() + kScaleX / 2, _rect.top() }, QPointF{ right, _rect.bottom() } }, Qt::AlignVCenter, _sequenceIndex);
		}

	private:
		const QString _sequenceIndex;
		const QRectF _rect;
		QBrush _brush{ Qt::white };
		QPen _pen{ Qt::black };
	};
}

CompositionScene::CompositionScene()
{
	setBackgroundBrush(kBackgroundColor);
}

void CompositionScene::reset(const aulos::Composition* composition)
{
	_cursorItem = nullptr;
	clear();

	if (!composition)
		return;

	const auto trackCount = composition->trackCount();
	if (!trackCount)
		return;

	double maxRight = 0.0;
	for (size_t trackIndex = 0; trackIndex < trackCount; ++trackIndex)
	{
		std::unordered_map<size_t, size_t> sequenceLengths;
		const auto sequenceCount = composition->sequenceCount(trackIndex);
		sequenceLengths.reserve(sequenceCount);
		for (size_t sequenceIndex = 0; sequenceIndex < sequenceCount; ++sequenceIndex)
		{
			const auto sequence = composition->sequence(trackIndex, sequenceIndex);
			size_t sequenceLength = 0;
			for (size_t i = 0; i < sequence._size; ++i)
				sequenceLength += sequence._data[i]._pause;
			sequenceLengths.emplace(sequenceIndex, sequenceLength);
		}

		std::vector<aulos::Fragment> fragments;
		const auto fragmentCount = composition->fragmentCount(trackIndex);
		fragments.reserve(fragmentCount);
		for (size_t i = 0; i < fragmentCount; ++i)
			fragments.emplace_back(composition->fragment(trackIndex, i));

		size_t offset = 0;
		for (const auto& fragment : fragments)
		{
			offset += fragment._delay;
			const auto duration = sequenceLengths[fragment._sequence];
			const auto item = new FragmentItem{ fragment._sequence, offset, trackIndex, duration };
			item->setBrush(kFragmentBackgroundColor[trackIndex % kFragmentBackgroundColor.size()]);
			item->setPen(kFragmentFrameColor[trackIndex % kFragmentFrameColor.size()]);
			addItem(item);
			maxRight = std::max(maxRight, item->boundingRect().right());
		}
	}

	maxRight += kScaleX; // So the end is clearly visible.

	for (size_t i = 0; i < trackCount; ++i)
	{
		const auto item = new QGraphicsRectItem{ 0.0, i * kScaleY, maxRight, kScaleY };
		item->setBrush(kTrackBackgroundColor[i % kTrackBackgroundColor.size()]);
		item->setPen(QColor{ Qt::transparent });
		item->setZValue(-1.0);
		addItem(item);
	}

	const auto totalHeight = trackCount * kScaleY;
	setSceneRect(0.0, 0.0, maxRight, totalHeight);

	_cursorItem = new QGraphicsLineItem{ 0.0, 0.0, 0.0, totalHeight };
	_cursorItem->setPen(kCursorColor);
	_cursorItem->setVisible(false);
	_cursorItem->setZValue(1.0);
	addItem(_cursorItem);
}

void CompositionScene::setCurrentStep(double step)
{
	if (_cursorItem)
	{
		const auto x = step * kScaleX;
		const auto isVisible = x >= 0.0 && x < sceneRect().right();
		_cursorItem->setVisible(isVisible);
		if (isVisible)
			_cursorItem->setTransform(QTransform{ 1.0, 0.0, 0.0, 1.0, step * kScaleX, 0.0 });
	}
}
