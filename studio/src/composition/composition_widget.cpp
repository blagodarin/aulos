// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#include "composition_widget.hpp"

#include "../theme.hpp"
#include "composition_scene.hpp"
#include "track_editor.hpp"
#include "voice_editor.hpp"

#include <aulos/data.hpp>

#include <cassert>

#include <QGraphicsView>
#include <QGridLayout>
#include <QMenu>
#include <QMessageBox>
#include <QScrollBar>

namespace
{
	const std::array<char, 12> kNoteNames{ 'C', 'C', 'D', 'D', 'E', 'F', 'F', 'G', 'G', 'A', 'A', 'B' };

	QFont makeBold(QFont&& font)
	{
		font.setBold(true);
		return std::move(font);
	}

	std::shared_ptr<aulos::VoiceData> makeDefaultVoice()
	{
		using namespace std::chrono_literals;
		auto voice = std::make_shared<aulos::VoiceData>();
		voice->_amplitudeEnvelope._changes = {
			{ 100ms, 1.f, aulos::EnvelopeShape::Linear },
			{ 400ms, .5f, aulos::EnvelopeShape::Linear },
			{ 500ms, 0.f, aulos::EnvelopeShape::Linear },
		};
		return voice;
	}

	QString makeSequenceName(const aulos::SequenceData& sequence)
	{
		QString result;
		for (const auto& sound : sequence._sounds)
		{
			if (!result.isEmpty())
				result += sound._delay > 0 ? ' ' : '|';
			for (size_t i = 1; i < sound._delay; ++i)
				result += ". ";
			const auto octave = QString::number(static_cast<uint8_t>(sound._note) / 12);
			const auto note = static_cast<size_t>(sound._note) % 12;
			result += kNoteNames[note];
			const bool isSharpNote = note > 0 && kNoteNames[note - 1] == kNoteNames[note];
			if (isSharpNote)
				result += '#';
			result += octave;
		}
		return result;
	}
}

CompositionWidget::CompositionWidget(QWidget* parent)
	: QWidget{ parent }
	, _voiceEditor{ std::make_unique<VoiceEditor>(this) }
	, _trackEditor{ std::make_unique<TrackEditor>(this) }
	, _scene{ new CompositionScene{ this } }
{
	const auto layout = new QGridLayout{ this };
	layout->setContentsMargins({});

	_view = new QGraphicsView{ _scene, this };
	_view->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
	layout->addWidget(_view, 0, 0);

	connect(_scene, &CompositionScene::fragmentMenuRequested, [this](const void* voiceId, const void* trackId, size_t offset, const QPoint& pos) {
		const auto part = std::find_if(_composition->_parts.cbegin(), _composition->_parts.cend(), [voiceId](const auto& partData) { return partData->_voice.get() == voiceId; });
		assert(part != _composition->_parts.cend());
		const auto track = std::find_if((*part)->_tracks.cbegin(), (*part)->_tracks.cend(), [trackId](const auto& trackData) { return trackData.get() == trackId; });
		assert(track != (*part)->_tracks.cend());
		const auto fragment = (*track)->_fragments.find(offset);
		assert(fragment != (*track)->_fragments.end());
		QMenu menu;
		const auto removeFragmentAction = menu.addAction(tr("Remove fragment"));
		menu.addSeparator();
		const auto editTrackAction = menu.addAction(tr("Edit track..."));
		const auto removeTrackAction = menu.addAction(tr("Remove track"));
		removeTrackAction->setEnabled((*part)->_tracks.size() > 1);
		if (const auto action = menu.exec(pos); action == removeFragmentAction)
		{
			_scene->removeFragment(trackId, offset);
			(*track)->_fragments.erase(fragment);
		}
		else if (action == editTrackAction)
		{
			if (!editTrack(**track))
				return;
		}
		else if (action == removeTrackAction)
		{
			if (const auto message = tr("Remove %1 track %2?").arg("<b>" + QString::fromStdString((*part)->_voiceName) + "</b>").arg(track - (*part)->_tracks.cbegin() + 1);
				QMessageBox::question(this, {}, message, QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
				return;
			_scene->removeTrack(voiceId, trackId);
			(*part)->_tracks.erase(track);
		}
		else
			return;
		emit compositionChanged();
	});
	connect(_scene, &CompositionScene::loopMenuRequested, [this](const QPoint& pos) {
		QMenu menu;
		menu.addAction(tr("Remove loop"));
		if (const auto action = menu.exec(pos))
		{
			_composition->_loopOffset = 0;
			_composition->_loopLength = 0;
		}
		else
			return;
		_scene->updateLoop();
		emit compositionChanged();
	});
	connect(_scene, &CompositionScene::newVoiceRequested, [this] {
		_voiceEditor->setVoiceName(tr("NewVoice").toStdString());
		if (_voiceEditor->exec() != QDialog::Accepted)
			return;
		const auto& part = _composition->_parts.emplace_back(std::make_shared<aulos::PartData>(::makeDefaultVoice()));
		part->_voiceName = _voiceEditor->voiceName();
		part->_tracks.emplace_back(std::make_shared<aulos::TrackData>(1));
		_scene->appendPart(part);
		_view->horizontalScrollBar()->setValue(_view->horizontalScrollBar()->minimum());
		emit compositionChanged();
	});
	connect(_scene, &CompositionScene::sequenceSelected, [this](const void* voiceId, const void* trackId, const void* sequenceId) {
		std::shared_ptr<aulos::VoiceData> voice;
		std::shared_ptr<aulos::SequenceData> sequence;
		if (voiceId)
		{
			const auto partIt = std::find_if(_composition->_parts.cbegin(), _composition->_parts.cend(), [voiceId](const auto& partData) { return partData->_voice.get() == voiceId; });
			assert(partIt != _composition->_parts.cend());
			voice = (*partIt)->_voice;
			if (sequenceId)
			{
				assert(trackId);
				const auto trackIt = std::find_if((*partIt)->_tracks.cbegin(), (*partIt)->_tracks.cend(), [trackId](const auto& trackData) { return trackData.get() == trackId; });
				assert(trackIt != (*partIt)->_tracks.cend());
				const auto sequenceIt = std::find_if((*trackIt)->_sequences.cbegin(), (*trackIt)->_sequences.cend(), [sequenceId](const auto& sequenceData) { return sequenceData.get() == sequenceId; });
				assert(sequenceIt != (*trackIt)->_sequences.end());
				sequence = *sequenceIt;
			}
		}
		emit selectionChanged(voice, sequence);
	});
	connect(_scene, &CompositionScene::timelineMenuRequested, [this](size_t step, const QPoint& pos) {
		const auto loopEnd = size_t{ _composition->_loopOffset } + _composition->_loopLength;
		QMenu menu;
		const auto beginAction = menu.addAction(tr("Begin loop here"));
		beginAction->setEnabled(step < loopEnd);
		const auto endAction = menu.addAction(tr("End loop here"));
		endAction->setEnabled(step >= _composition->_loopOffset);
		if (const auto action = menu.exec(pos); action == beginAction)
		{
			_composition->_loopOffset = static_cast<unsigned>(step);
			_composition->_loopLength = static_cast<unsigned>(loopEnd - step);
		}
		else if (action == endAction)
			_composition->_loopLength = static_cast<unsigned>(step - _composition->_loopOffset + 1);
		else
			return;
		_scene->updateLoop();
		emit compositionChanged();
	});
	connect(_scene, &CompositionScene::trackActionRequested, [this](const void* voiceId, const void* trackId) {
		const auto part = std::find_if(_composition->_parts.cbegin(), _composition->_parts.cend(), [voiceId](const auto& partData) { return partData->_voice.get() == voiceId; });
		assert(part != _composition->_parts.cend());
		const auto track = std::find_if((*part)->_tracks.cbegin(), (*part)->_tracks.cend(), [trackId](const auto& trackData) { return trackData.get() == trackId; });
		assert(track != (*part)->_tracks.cend());
		if (!editTrack(**track))
			return;
		emit compositionChanged();
	});
	connect(_scene, &CompositionScene::trackMenuRequested, [this](const void* voiceId, const void* trackId, size_t offset, const QPoint& pos) {
		const auto part = std::find_if(_composition->_parts.cbegin(), _composition->_parts.cend(), [voiceId](const auto& partData) { return partData->_voice.get() == voiceId; });
		assert(part != _composition->_parts.cend());
		const auto track = std::find_if((*part)->_tracks.cbegin(), (*part)->_tracks.cend(), [trackId](const auto& trackData) { return trackData.get() == trackId; });
		assert(track != (*part)->_tracks.cend());
		QMenu menu;
		const auto editTrackAction = menu.addAction(tr("Edit track..."));
		editTrackAction->setFont(::makeBold(editTrackAction->font()));
		const auto insertSubmenu = menu.addMenu(tr("Insert sequence"));
		const auto sequenceCount = (*track)->_sequences.size();
		for (size_t sequenceIndex = 0; sequenceIndex < sequenceCount; ++sequenceIndex)
			insertSubmenu->addAction(::makeSequenceName(*(*track)->_sequences[sequenceIndex]))->setData(static_cast<unsigned>(sequenceIndex));
		if (!(*track)->_sequences.empty())
			insertSubmenu->addSeparator();
		const auto newSequenceAction = insertSubmenu->addAction(tr("New sequence..."));
		const auto removeTrackAction = menu.addAction(tr("Remove track"));
		removeTrackAction->setEnabled((*part)->_tracks.size() > 1);
		if (const auto action = menu.exec(pos); action == editTrackAction)
		{
			if (!editTrack(**track))
				return;
		}
		else if (action == newSequenceAction)
		{
			const auto sequence = std::make_shared<aulos::SequenceData>();
			(*track)->_sequences.emplace_back(sequence);
			[[maybe_unused]] const auto inserted = (*track)->_fragments.insert_or_assign(offset, sequence).second;
			assert(inserted);
			_scene->insertFragment(voiceId, trackId, offset, sequence);
			_scene->selectSequence(voiceId, trackId, sequence.get());
		}
		else if (action == removeTrackAction)
		{
			if (const auto message = tr("Remove %1 track %2?").arg("<b>" + QString::fromStdString((*part)->_voiceName) + "</b>").arg(track - (*part)->_tracks.cbegin() + 1);
				QMessageBox::question(this, {}, message, QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
				return;
			_scene->removeTrack(voiceId, trackId);
			(*part)->_tracks.erase(track);
		}
		else if (action)
		{
			const auto& sequence = (*track)->_sequences[action->data().toUInt()];
			[[maybe_unused]] const auto inserted = (*track)->_fragments.insert_or_assign(offset, sequence).second;
			assert(inserted);
			_scene->insertFragment(voiceId, trackId, offset, sequence);
		}
		else
			return;
		emit compositionChanged();
	});
	connect(_scene, &CompositionScene::voiceActionRequested, [this](const void* voiceId) {
		const auto part = std::find_if(_composition->_parts.cbegin(), _composition->_parts.cend(), [voiceId](const auto& partData) { return partData->_voice.get() == voiceId; });
		assert(part != _composition->_parts.cend());
		if (!editVoiceName(voiceId, (*part)->_voiceName))
			return;
		emit compositionChanged();
	});
	connect(_scene, &CompositionScene::voiceMenuRequested, [this](const void* voiceId, const QPoint& pos) {
		const auto part = std::find_if(_composition->_parts.cbegin(), _composition->_parts.cend(), [voiceId](const auto& partData) { return partData->_voice.get() == voiceId; });
		assert(part != _composition->_parts.cend());
		QMenu menu;
		const auto editVoiceAction = menu.addAction(tr("Rename voice..."));
		editVoiceAction->setFont(::makeBold(editVoiceAction->font()));
		const auto addTrackAction = menu.addAction(tr("Add track"));
		menu.addSeparator();
		const auto removeVoiceAction = menu.addAction(tr("Remove voice"));
		if (const auto action = menu.exec(pos); action == editVoiceAction)
		{
			if (!editVoiceName(voiceId, (*part)->_voiceName))
				return;
		}
		else if (action == addTrackAction)
		{
			auto& track = (*part)->_tracks.emplace_back(std::make_shared<aulos::TrackData>(1));
			_scene->addTrack(voiceId, track.get());
		}
		else if (action == removeVoiceAction)
		{
			if (const auto message = tr("Remove %1 voice?").arg("<b>" + QString::fromStdString((*part)->_voiceName) + "</b>");
				QMessageBox::question(this, {}, message, QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
				return;
			_scene->removeVoice(voiceId);
			_composition->_parts.erase(part);
		}
		else
			return;
		emit compositionChanged();
	});
}

float CompositionWidget::selectedTrackWeight() const
{
	return _scene->selectedTrackWeight();
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
	const auto sceneCursorRect = _scene->setCurrentStep(step);
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

void CompositionWidget::setSpeed(unsigned speed)
{
	_scene->setSpeed(speed);
}

void CompositionWidget::showCursor(bool visible)
{
	_scene->showCursor(visible);
}

size_t CompositionWidget::startOffset() const
{
	return _scene->startOffset();
}

void CompositionWidget::updateSelectedSequence(const std::shared_ptr<aulos::SequenceData>& sequence)
{
	_scene->updateSelectedSequence(sequence);
}

bool CompositionWidget::editTrack(aulos::TrackData& track)
{
	_trackEditor->setTrackWeight(track._properties->_weight);
	if (_trackEditor->exec() != QDialog::Accepted)
		return false;
	track._properties->_weight = _trackEditor->trackWeight();
	return true;
}

bool CompositionWidget::editVoiceName(const void* id, std::string& voiceName)
{
	_voiceEditor->setVoiceName(voiceName);
	if (_voiceEditor->exec() != QDialog::Accepted)
		return false;
	voiceName = _voiceEditor->voiceName();
	_scene->updateVoice(id, voiceName);
	_view->horizontalScrollBar()->setValue(_view->horizontalScrollBar()->minimum());
	return true;
}
