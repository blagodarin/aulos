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

#include <QGraphicsItem>
#include <QStaticText>

namespace aulos
{
	struct CompositionData;
}

class VoiceItem : public QGraphicsItem
{
public:
	VoiceItem(const std::shared_ptr<const aulos::CompositionData>&, size_t trackIndex, QGraphicsItem* parent = nullptr);

	QRectF boundingRect() const override { return _rect; }
	void paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*) override;
	qreal requiredWidth() const;
	void setWidth(qreal);

private:
	const std::shared_ptr<const aulos::CompositionData> _composition;
	const size_t _trackIndex;
	QStaticText _name;
	QRectF _rect;
};