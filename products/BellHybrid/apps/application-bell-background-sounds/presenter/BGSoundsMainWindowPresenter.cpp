// Copyright (c) 2017-2021, Mudita Sp. z.o.o. All rights reserved.
// For licensing, see https://github.com/mudita/MuditaOS/LICENSE.md

#include "BGSoundsMainWindowPresenter.hpp"
#include <apps-common/models/SongsRepository.hpp>
#include <purefs/filesystem_paths.hpp>

namespace app::bgSounds
{
    auto bgSoundsPath = purefs::dir::getCurrentOSPath() / "assets" / "audio" / "bell" / "bg_sounds";

    BGSoundsMainWindowPresenter::BGSoundsMainWindowPresenter(
        std::shared_ptr<app::music::AbstractSongsRepository> soundsRepository)
        : soundsRepository{std::move(soundsRepository)}
    {}
    void BGSoundsMainWindowPresenter::loadAudioRecords()
    {
        soundsRepository->getMusicFilesList(
            0,
            100,
            [this](const std::vector<db::multimedia_files::MultimediaFilesRecord> &records,
                   unsigned int repoRecordsCount) {
                getView()->setSoundsList(records);
                return true;
            });
    }
} // namespace app::bgSounds
