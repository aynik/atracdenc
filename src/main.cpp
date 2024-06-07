/*
 * This file is part of AtracDEnc.
 *
 * AtracDEnc is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * AtracDEnc is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with AtracDEnc; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <iostream>
#include <string>
#include <stdexcept>

#include <getopt.h>

#include "help.h"

#include "pcmengin.h"
#include "wav.h"
#include "aea.h"
#include "config.h"
#include "atrac1denc.h"

using std::cout;
using std::cerr;
using std::endl;
using std::string;
using std::stoi;

using namespace NAtracDEnc;

typedef std::unique_ptr<TPCMEngine<TFloat>> TPcmEnginePtr;
typedef std::unique_ptr<IProcessor<TFloat>> TAtracProcessorPtr;

static void printUsage(const char* myName, const string& err = string())
{
    if (!err.empty()) {
        cerr << err << endl;
    }
    cerr << "\tuse: " << myName << " -h to get help" << endl;
}

static void printProgress(int percent)
{
    static uint32_t counter;
    counter++;
    const char symbols[4] = {'-', '\\', '|', '/'};
    cout << symbols[counter % 4]<< "  "<< percent <<"% done\r";
    fflush(stdout);
}

static int checkedStoi(const char* data, int min, int max, int def)
{
    int tmp = 0;
    try {
        tmp = stoi(data);
        if (tmp < min || tmp > max)
            throw std::invalid_argument(data);
        return tmp;
    } catch (std::invalid_argument&) {
        cerr << "Wrong arg: " << data << " " << def << " will be used" << endl;
        return def;
    }
}

enum EOptions
{
    O_ENCODE = 'e',
    O_DECODE = 'd',
    O_TEST = 't',
    O_HELP = 'h',
    O_BITRATE = 'b',
    O_BFUIDXCONST = 1,
    O_BFUIDXFAST = 2,
    O_NOTRANSIENT = 3,
    O_MONO = 'm',
    O_NOSTDOUT = '4',
    O_NOTONAL = 5,
    O_NOGAINCONTROL = 6,
};

static void CheckInputFormat(const TWav* p)
{
    if (p->GetSampleRate() != 44100)
        throw std::runtime_error("unsupported sample rate");
}

static void PrepareAtrac1Encoder(const string& inFile,
                                 const string& outFile,
                                 const bool noStdOut,
                                 NAtrac1::TAtrac1EncodeSettings&& encoderSettings,
                                 uint64_t* totalSamples,
                                 TWavPtr* wavIO,
                                 TPcmEnginePtr* pcmEngine,
                                 TAtracProcessorPtr* atracProcessor)
{
    using NAtrac1::TAtrac1Data;

    {
        TWav* wavPtr = new TWav(inFile);
        CheckInputFormat(wavPtr);
        wavIO->reset(wavPtr);
    }
    const uint8_t numChannels = (*wavIO)->GetChannelNum();
    *totalSamples = (*wavIO)->GetTotalSamples();
    const uint64_t numFrames = numChannels * (*totalSamples) / TAtrac1Data::NumSamples;
    if (numFrames >= UINT32_MAX) {
        std::cerr << "Number of input samples exceeds output format limitation,"
            "the result will be incorrect" << std::endl;
    }
    TCompressedOutputPtr aeaIO = CreateAeaOutput(outFile, "test", numChannels, (uint32_t)numFrames);
    pcmEngine->reset(new TPCMEngine<TFloat>(4096,
                                            numChannels,
                                            TPCMEngine<TFloat>::TReaderPtr((*wavIO)->GetPCMReader<TFloat>())));
    if (!noStdOut)
        cout << "Input\n Filename: " << inFile
             << "\n Channels: " << (int)numChannels
             << "\n SampleRate: " << (*wavIO)->GetSampleRate()
             << "\n Duration (sec): " << *totalSamples / (*wavIO)->GetSampleRate()
	     << "\nOutput:\n Filename: " << outFile
	     << "\n Codec: ATRAC1"
             << endl;
    atracProcessor->reset(new TAtrac1Encoder(std::move(aeaIO), std::move(encoderSettings)));
}

static void PrepareAtrac1Decoder(const string& inFile,
                                 const string& outFile,
                                 const bool noStdOut,
                                 uint64_t* totalSamples,
                                 TWavPtr* wavIO,
                                 TPcmEnginePtr* pcmEngine,
                                 TAtracProcessorPtr* atracProcessor)
{
    TCompressedInputPtr aeaIO = CreateAeaInput(inFile);
    *totalSamples = aeaIO->GetLengthInSamples();
    if (!noStdOut)
        cout << "Input\n Filename: " << inFile
             << "\n Name: " << aeaIO->GetName()
             << "\n Channels: " << (int)aeaIO->GetChannelNum()
	     << "\nOutput:\n Filename: " << outFile
	     << "\n Codec: PCM"
             << endl;
    wavIO->reset(new TWav(outFile, aeaIO->GetChannelNum(), 44100));
    pcmEngine->reset(new TPCMEngine<TFloat>(4096,
                                            aeaIO->GetChannelNum(),
                                            TPCMEngine<TFloat>::TWriterPtr((*wavIO)->GetPCMWriter<TFloat>())));
    atracProcessor->reset(new TAtrac1Decoder(std::move(aeaIO)));
}

void encode(const string& inFile, const string& outFile, const bool noStdOut, uint32_t bfuIdxConst, bool fastBfuNumSearch, NAtrac1::TAtrac1EncodeSettings::EWindowMode windowMode, uint32_t winMask)
{
    TPcmEnginePtr pcmEngine;
    TAtracProcessorPtr atracProcessor;
    uint64_t totalSamples = 0;
    TWavPtr wavIO;
    uint32_t pcmFrameSz = 0;

    try {
        if (bfuIdxConst > 8) {
            throw std::invalid_argument("ATRAC1 mode, --bfuidxconst is a index of max used BFU. "
                "Values [1;8] is allowed");
        }
        using NAtrac1::TAtrac1Data;
        NAtrac1::TAtrac1EncodeSettings encoderSettings(bfuIdxConst, fastBfuNumSearch, windowMode, winMask);
        PrepareAtrac1Encoder(inFile, outFile, noStdOut, std::move(encoderSettings),
            &totalSamples, &wavIO, &pcmEngine, &atracProcessor);
        pcmFrameSz = TAtrac1Data::NumSamples;
    } catch (const std::exception& ex) {
        cerr << "Fatal error: " << ex.what() << endl;
        return;
    }

    auto atracLambda = atracProcessor->GetLambda();

    uint64_t processed = 0;
    try {
        while (totalSamples > (processed = pcmEngine->ApplyProcess(pcmFrameSz, atracLambda))) {
            if (!noStdOut)
                printProgress(static_cast<int>(processed * 100 / totalSamples));
        }
        if (!noStdOut)
            cout << "\nDone" << endl;
    } catch (const TAeaIOError& err) {
        cerr << "Aea IO fatal error: " << err.what() << endl;
        return;
    } catch (const TNoDataToRead&) {
        cerr << "No more data to read from input" << endl;
        return;
    } catch (const std::exception& ex) {
        cerr << "Encode error: " << ex.what() << endl;
        return;
    }
}

void decode(const string& inFile, const string& outFile, const bool noStdOut)
{
    TPcmEnginePtr pcmEngine;
    TAtracProcessorPtr atracProcessor;
    uint64_t totalSamples = 0;
    TWavPtr wavIO;
    uint32_t pcmFrameSz = 0;

    try {
        using NAtrac1::TAtrac1Data;
        PrepareAtrac1Decoder(inFile, outFile, noStdOut,
            &totalSamples, &wavIO, &pcmEngine, &atracProcessor);
        pcmFrameSz = TAtrac1Data::NumSamples;
    } catch (const std::exception& ex) {
        cerr << "Fatal error: " << ex.what() << endl;
        return;
    }

    auto atracLambda = atracProcessor->GetLambda();

    uint64_t processed = 0;
    try {
        while (totalSamples > (processed = pcmEngine->ApplyProcess(pcmFrameSz, atracLambda))) {
            if (!noStdOut)
                printProgress(static_cast<int>(processed * 100 / totalSamples));
        }
        if (!noStdOut)
            cout << "\nDone" << endl;
    } catch (const TAeaIOError& err) {
        cerr << "Aea IO fatal error: " << err.what() << endl;
        return;
    } catch (const TNoDataToRead&) {
        cerr << "No more data to read from input" << endl;
        return;
    } catch (const std::exception& ex) {
        cerr << "Decode error: " << ex.what() << endl;
        return;
    }
}

int main(int argc, char* const* argv)
{
    const char* myName = argv[0];
    static struct option longopts[] = {
        { "encode", no_argument, NULL, O_ENCODE },
        { "decode", no_argument, NULL, O_DECODE },
        { "test", no_argument, NULL, O_TEST },
        { "help", no_argument, NULL, O_HELP },
        { "bfuidxconst", required_argument, NULL, O_BFUIDXCONST },
        { "bfuidxfast", no_argument, NULL, O_BFUIDXFAST },
        { "notransient", optional_argument, NULL, O_NOTRANSIENT },
        { "nostdout", no_argument, NULL, O_NOSTDOUT },
        { NULL, 0, NULL, 0 }
    };

    int ch = 0;
    string inFile;
    string outFile;
    string medFile;
    uint32_t mode = 0;
    uint32_t bfuIdxConst = 0; // 0 - auto, no const
    bool fastBfuNumSearch = false;
    bool noStdOut = false;
    NAtrac1::TAtrac1EncodeSettings::EWindowMode windowMode = NAtrac1::TAtrac1EncodeSettings::EWindowMode::EWM_AUTO;
    uint32_t winMask = 0; // 0 - all is long

    while ((ch = getopt_long(argc, argv, "edthi:o:m:", longopts, NULL)) != -1) {
        switch (ch) {
            case O_ENCODE:
                mode |= E_ENCODE;
                break;
            case O_DECODE:
                mode |= E_DECODE;
                break;
            case O_TEST:
                mode |= E_TEST;
                break;
            case 'i':
                inFile = optarg;
                break;
            case 'o':
                outFile = optarg;
                if (outFile == "-") {
                    noStdOut = true;
                }
                break;
            case 'm':
                medFile = optarg;
                break;
            case 'h':
                cout << GetHelp() << endl;
                return 0;
                break;
            case O_BFUIDXCONST:
                bfuIdxConst = checkedStoi(optarg, 1, 32, 0);
                break;
            case O_BFUIDXFAST:
                fastBfuNumSearch = true;
                break;
            case O_NOTRANSIENT:
                windowMode = NAtrac1::TAtrac1EncodeSettings::EWindowMode::EWM_NOTRANSIENT;
                if (optarg) {
                    winMask = stoi(optarg);
                }
                cout << "Transient detection disabled, bands: low - "
                     << ((winMask & 1) ? "short" : "long") << ", mid - "
                     << ((winMask & 2) ? "short" : "long") << ", hi - "
                     << ((winMask & 4) ? "short" : "long") << endl;
                break;
            case O_NOSTDOUT:
                noStdOut = true;
                break;
            default:
                printUsage(myName);
                return 1;
        }
    }
    argc -= optind;
    argv += optind;

    if (argc != 0) {
        cerr << "Unhandled arg: " << argv[0] << endl;
        return 1;
    }

    if (inFile.empty()) {
        cerr << "No input file" << endl;
        return 1;
    }
    if (outFile.empty()) {
        cerr << "No output file" << endl;
        return 1;
    }
    if (mode == E_TEST && medFile.empty()) {
        cerr << "No intermediate file" << endl;
        return 1;
    }

    switch (mode) {
        case E_ENCODE:
            encode(inFile, outFile, noStdOut, bfuIdxConst, fastBfuNumSearch, windowMode, winMask);
            break;
        case E_DECODE:
            decode(inFile, outFile, noStdOut);
            break;
        case E_TEST:
            encode(inFile, medFile, noStdOut, bfuIdxConst, fastBfuNumSearch, windowMode, winMask);
            decode(medFile, outFile, noStdOut);
            break;
        default:
            cerr << "Processing mode was not specified" << endl;
            return 1;
    }

    return 0;
}
