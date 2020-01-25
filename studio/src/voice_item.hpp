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

#include <QGraphicsObject>
#include <QStaticText>

namespace aulos
{
	struct Voice;
}

class VoiceItem final : public QGraphicsObject
{
	Q_OBJECT

public:
	VoiceItem(const std::shared_ptr<aulos::Voice>&, QGraphicsItem* parent = nullptr);

	QRectF boundingRect() const override;
	void paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*) override;
	qreal requiredWidth() const;
	void setIndex(size_t);
	void setTrackCount(size_t);
	void setVoiceName(const QString& name);
	void setWidth(qreal);
	const aulos::Voice* voiceId() const noexcept { return _voice.get(); }

signals:
	void voiceEditRequested(const std::shared_ptr<aulos::Voice>&);

private:
	void contextMenuEvent(QGraphicsSceneContextMenuEvent*) override;

private:
	size_t _index = 0;
	qreal _width = 0;
	size_t _trackCount = 0;
	const std::shared_ptr<aulos::Voice> _voice;
	QStaticText _name;
};
