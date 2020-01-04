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

#include "voices_model.hpp"

#include <aulos.hpp>

void VoicesModel::reset(const aulos::Composition* composition)
{
	beginResetModel();
	_voices.clear();
	if (composition)
		for (size_t i = 0; i < composition->voiceCount(); ++i)
			_voices << composition->voice(i);
	endResetModel();
}

void VoicesModel::setVoice(const QModelIndex& index, const aulos::Voice& voice)
{
	if (!index.isValid())
		return;
	_voices[index.row()] = voice;
	emit dataChanged(index, index);
}

const aulos::Voice* VoicesModel::voice(const QModelIndex& index) const
{
	return index.isValid() ? &_voices[index.row()] : nullptr;
}

QVariant VoicesModel::data(const QModelIndex& index, int role) const
{
	return index.isValid() && role == Qt::DisplayRole ? QString::fromStdString(_voices[index.row()]._name) : QVariant{};
}

QVariant VoicesModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (orientation != Qt::Vertical || role != Qt::DisplayRole)
		return {};
	return QString::number(section + 1);
}

QModelIndex VoicesModel::index(int row, int column, const QModelIndex& parent) const
{
	return !parent.isValid() && row >= 0 && row < _voices.size() && column == 0 ? createIndex(row, column) : QModelIndex{};
}
