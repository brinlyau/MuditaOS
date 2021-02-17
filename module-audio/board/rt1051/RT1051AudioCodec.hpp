// Copyright (c) 2017-2021, Mudita Sp. z.o.o. All rights reserved.
// For licensing, see https://github.com/mudita/MuditaOS/LICENSE.md

#pragma once

#include "SAIAudioDevice.hpp"
#include "fsl_sai_edma.h"

#include "FreeRTOS.h"
#include "task.h"
#include "macros.h"

#include "board/rt1051/bsp/audio/CodecMAX98090.hpp"

#include "drivers/pll/DriverPLL.hpp"
#include "drivers/dmamux/DriverDMAMux.hpp"
#include "drivers/dma/DriverDMA.hpp"

#include <mutex.hpp>

namespace audio
{

    void txAudioCodecCallback(I2S_Type *base, sai_edma_handle_t *handle, status_t status, void *userData);
    void rxAudioCodecCallback(I2S_Type *base, sai_edma_handle_t *handle, status_t status, void *userData);

    class RT1051AudioCodec : public SAIAudioDevice
    {

      public:
        friend void txAudioCodecCallback(I2S_Type *base, sai_edma_handle_t *handle, status_t status, void *userData);
        friend void rxAudioCodecCallback(I2S_Type *base, sai_edma_handle_t *handle, status_t status, void *userData);

        RT1051AudioCodec();
        virtual ~RT1051AudioCodec();

        AudioDevice::RetCode Start(const Format &format) override final;
        AudioDevice::RetCode Stop() override final;
        AudioDevice::RetCode OutputVolumeCtrl(float vol) override final;
        AudioDevice::RetCode InputGainCtrl(float gain) override final;
        AudioDevice::RetCode OutputPathCtrl(OutputPath outputPath) override final;
        AudioDevice::RetCode InputPathCtrl(InputPath inputPath) override final;
        bool IsFormatSupported(const Format &format) override final;

        cpp_freertos::MutexStandard mutex;

      private:
        constexpr static TickType_t codecSettleTime = 20 * portTICK_PERIOD_MS;

        enum class State
        {
            Running,
            Stopped
        };

        struct SAIFormat
        {
            uint32_t sampleRate_Hz;   /*!< Sample rate of audio data */
            uint32_t bitWidth;        /*!< Data length of audio data, usually 8/16/24/32 bits */
            sai_mono_stereo_t stereo; /*!< Mono or stereo */
        };

        State state = State::Stopped;
        SAIFormat saiInFormat;
        SAIFormat saiOutFormat;
        CodecParamsMAX98090 codecParams;
        CodecMAX98090 codec;

        static AT_NONCACHEABLE_SECTION_INIT(sai_edma_handle_t txHandle);
        static AT_NONCACHEABLE_SECTION_INIT(sai_edma_handle_t rxHandle);

        void OutStart();
        void InStart();
        void OutStop();
        void InStop();

        CodecParamsMAX98090::InputPath getCodecInputPath(const AudioDevice::Format &format);
        CodecParamsMAX98090::OutputPath getCodecOutputPath(const AudioDevice::Format &format);
    };
} // namespace audio