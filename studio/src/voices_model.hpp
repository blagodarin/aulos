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

#include <QAbstractItemModel>

namespace aulos
{
	class Composition;
	struct Voice;
}

class VoicesModel : public QAbstractItemModel
{
	Q_OBJECT

public:
	QModelIndex addVoice(const aulos::Voice&);
	void removeVoice(const QModelIndex&);
	void reset(const aulos::Composition*);
	void setVoice(const QModelIndex&, const aulos::Voice&);
	const aulos::Voice* voice(const QModelIndex&) const;

private:
	int columnCount(const QModelIndex&) const override { return 1; }
	QVariant data(const QModelIndex& index, int role) const override;
	Qt::ItemFlags flags(const QModelIndex&) const override { return Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled; }
	QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
	QModelIndex index(int row, int column, const QModelIndex& parent) const override;
	QModelIndex parent(const QModelIndex&) const override { return {}; }
	int rowCount(const QModelIndex& parent) const override { return parent.isValid() ? 0 : _voices.size(); }
	bool setData(const QModelIndex& index, const QVariant& value, int role) override;

private:
	QList<aulos::Voice> _voices;
};
