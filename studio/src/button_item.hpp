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

#include <QGraphicsObject>

class ButtonItem : public QGraphicsObject
{
	Q_OBJECT

public:
	enum class Mode
	{
		Click,
		Press,
	};

	explicit ButtonItem(Mode mode = Mode::Click, QGraphicsItem* parent = nullptr);

signals:
	void activated();

protected:
	bool isHovered() const noexcept { return _hovered; }
	bool isPressed() const noexcept { return _pressed; }

private:
	void hoverEnterEvent(QGraphicsSceneHoverEvent*) override;
	void hoverLeaveEvent(QGraphicsSceneHoverEvent*) override;
	void mousePressEvent(QGraphicsSceneMouseEvent*) override;
	void mouseReleaseEvent(QGraphicsSceneMouseEvent*) override;

private:
	const Mode _mode;
	bool _hovered = false;
	bool _pressed = false;
};
