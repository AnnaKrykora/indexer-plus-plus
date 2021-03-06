// This file is the part of the Indexer++ project.
// Copyright (C) 2016 Anna Krykora <krykoraanna@gmail.com>. All rights reserved.
// Use of this source code is governed by a MIT-style license that can be found in the LICENSE file.

#include "USNJournalRecordsSerializer.h"

#include <fcntl.h>
#include <stdio.h>

#include "../Common/Helpers.h"
#include "Macros.h"
#include "WindowsWrapper.h"

#include "CommandlineArguments.h"
#include "Helpers.h"
#include "IndexerDateTime.h"

namespace ntfs_reader {

	using namespace indexer_common;

    USNJournalRecordsSerializer& USNJournalRecordsSerializer::Instance() {
        static USNJournalRecordsSerializer instance_;
        return instance_;
    }

    USNJournalRecordsSerializer::USNJournalRecordsSerializer() : records_db_(nullptr) {
        if (!CommandlineArguments::Instance().SaveUSNJournalRecords) return;

		auto filename = kFilenamePrefix + std::to_string(IndexerDateTime::TicksNow()) + ".txt";

        records_db_ = fopen(filename.c_str(), "w");
#ifdef WIN32
        _setmode(_fileno(records_db_), _O_U8TEXT);
#endif

#ifndef SINGLE_THREAD_LOG
		NEW_MUTEX
		worker_ = new std::thread(&USNJournalRecordsSerializer::WriteToFileAsync, this);
#endif  //!SINGLE_THREAD_LOG
    }

    USNJournalRecordsSerializer::~USNJournalRecordsSerializer() {
#ifndef SINGLE_THREAD_LOG
		DELETE_MUTEX
#endif  //!SINGLE_THREAD_LOG

        if (records_db_) {
            fclose(records_db_);
        }
    }


    void USNJournalRecordsSerializer::SerializeRecord(const USN_RECORD& record, char drive_letter) {
        auto res = helpers::SerializeRecord(record, drive_letter);

#ifdef SINGLE_THREAD_LOG
        WriteToFile(std::move(res));
#else
		PLOCK_GUARD
        records_.push_back(std::move(res));
#endif  // SINGLE_THREAD_LOG
    }

	void USNJournalRecordsSerializer::DeserializeAllRecords(const std::wstring& records_filename) {
		std::vector<std::unique_ptr<USNJournalRecordsProvider>> providers;

        for (auto i = 0; i < 26; ++i) {
			providers.push_back(std::make_unique<USNJournalRecordsProvider>('A' + i));
        }
#ifdef WIN32
        FILE* records_in = _wfopen(records_filename.c_str(), L"r");
        _setmode(_fileno(records_in), _O_U8TEXT);
#else
        FILE* records_in = fopen(helpers::WStringToString(records_filename).c_str(), "r");
#endif

        wchar_t buffer[1001];

        while (fgetws(buffer, 1000, records_in)) {
            auto record_drive_pair = helpers::DeserializeRecord(buffer);
            providers[record_drive_pair.second - 'A']->AddRecord(std::move(record_drive_pair.first));
        }

        fclose(records_in);

        for (auto i = 0; i < 26; ++i) {
            if (!providers[i]->Empty()) {
				records_providers_[providers[i]->DriveLetter()] = std::move(providers[i]);
            }
        }
    }

    USNJournalRecordsProvider* USNJournalRecordsSerializer::GetRecordsProvider(char drive_letter) {
        if (records_providers_.find(drive_letter) == records_providers_.end()) return nullptr;

        return records_providers_[drive_letter].get();
    }


#ifdef SINGLE_THREAD_LOG

    void USNJournalRecordsSerializer::WriteToFile(std::wstring msg) {
        fwprintf(records_db_, L"%s\n", (msg).c_str());
        fflush(records_db_);
    }

#else

    void USNJournalRecordsSerializer::WriteToFileAsync() {
        while (true) {
			std::this_thread::sleep_for(std::chrono::seconds(2));

            PLOCK
            swap(records_, tmp_records_storage_);
            PUNLOCK

            for (const auto& msg : tmp_records_storage_) {
                fwprintf(records_db_, L"%s\n", msg.c_str());
            }

            tmp_records_storage_.clear();
            fflush(records_db_);
        }
    }

#endif  // SINGLE_THREAD_LOG

} // namespace ntfs_reader