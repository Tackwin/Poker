#include "OS/file.hpp"
#include <thread>
#include <cassert>
#include <Windows.h>
#include <ShObjIdl_core.h>

#include "macros.hpp"

namespace Error {
	constexpr auto No_Error = 0;
#define X(x) constexpr auto x = 1 + __COUNTER__
	X(Shader_Fragment_Load);
	X(Shader_Vertex_Load);
	X(Win_Create_File);
	X(Win_File_Size);
	X(Win_File_Read);
	X(Web_File_Read);
	X(Win_File_Write);
	X(Win_File_Incomplete_Write);
	X(Unsupported_Operation);
#undef X
}

xstd::std_expected<std::string>
file::read_whole_text(const std::filesystem::path& path) noexcept {
	auto handle = CreateFile(
		path.string().c_str(),
		GENERIC_READ,
		FILE_SHARE_READ,
		nullptr,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		nullptr
	);
	if (handle == INVALID_HANDLE_VALUE) {
		return Error::Win_Create_File;
	}
	defer{ CloseHandle(handle); };

	LARGE_INTEGER large_int;
	GetFileSizeEx(handle, &large_int);
	if (large_int.QuadPart == 0) {
		return Error::Win_File_Size;
	}

	std::string buffer;
	buffer.resize((size_t)large_int.QuadPart);

	DWORD read;
	if (!ReadFile(handle, buffer.data(), (DWORD)large_int.QuadPart, &read, nullptr)) {
		return Error::Win_File_Read;
	}

	return buffer;
}

xstd::std_expected<std::vector<std::uint8_t>>
file::read_whole_file(const std::filesystem::path& path) noexcept {
	auto handle = CreateFile(
		path.string().c_str(),
		GENERIC_READ,
		FILE_SHARE_READ,
		nullptr,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		nullptr
	);
	if (handle == INVALID_HANDLE_VALUE) {
		auto err = GetLastError();

		

		printf("Erreur create file. %lu\n", err);
		return Error::Win_Create_File;
	}
	defer{ CloseHandle(handle); };
    
	LARGE_INTEGER large_int;
	GetFileSizeEx(handle, &large_int);
	if (large_int.QuadPart == 0) {
		return Error::Win_File_Size;
	}
    
	std::vector<std::uint8_t> buffer;
	buffer.resize((size_t)large_int.QuadPart);
    
	DWORD read;
	if (!ReadFile(handle, buffer.data(), (DWORD)large_int.QuadPart, &read, nullptr)) {
		return Error::Win_File_Read;
	}
    
	return buffer;
}

size_t file::overwrite_file_byte(
std::filesystem::path path, const std::vector<std::uint8_t>& bytes
) noexcept {
	FILE* f;
	auto err = fopen_s(&f, path.generic_string().c_str(), "wb");
	if (!f || err) return Error::Win_File_Write;

	defer{ fclose(f); };

	auto wrote = fwrite(bytes.data(), 1, bytes.size(), f);
	if (wrote != bytes.size()) return Error::Win_File_Incomplete_Write;

	return Error::No_Error;
}

bool file::overwrite_file(const std::filesystem::path& path, std::string_view str) noexcept {
	FILE* f;
	auto err = fopen_s(&f, path.generic_string().c_str(), "wb");
	if (!f || err) return false;

	defer{ fclose(f); };

	auto wrote = fwrite(str.data(), 1, str.size(), f);
	if (wrote != str.size()) return false;

	return true;
}
const char* create_cstr_extension_label_map(
decltype(file::OpenFileOpts::ext_filters) filters
) noexcept {
	std::string result;

	for (auto& [label, exts] : filters) {
		result += label + '\0';

		for (auto& ext : exts) {
			result += ext + ';';
		}

		if (!exts.empty()) {
			result.pop_back();
		}

		result += '\0';
	}

	char* result_cstr = new char[result.size() + 1];
	memcpy(result_cstr, result.c_str(), sizeof(char) * (result.size() + 1));
	result_cstr[result.size()] = '\0';
	return result_cstr;
}

void file::open_file_async(
std::function<void(file::OpenFileResult)>&& callback, file::OpenFileOpts opts
) noexcept {
	auto thread = std::thread([opts, callback]() {
		constexpr auto BUFFER_SIZE = 512;

		char* filepath = new char[BUFFER_SIZE];
		memcpy(
			filepath,
			opts.filepath.string().c_str(),
			opts.filepath.string().size() + 1
		);
		defer{ delete[] filepath; };

		char* filename = new char[BUFFER_SIZE];
		memcpy(
			filename,
			opts.filename.string().c_str(),
			opts.filename.string().size() + 1
		);
		defer{ delete[] filename; };

		const char* filters = create_cstr_extension_label_map(opts.ext_filters);
		defer{ delete filters; };

		OPENFILENAME ofn;
		ZeroMemory(&ofn, sizeof(ofn));

		ofn.lStructSize = sizeof(OPENFILENAME);
		ofn.hwndOwner = (HWND)opts.owner;
		ofn.lpstrFilter = filters;
		ofn.lpstrFile = filepath;
		ofn.nMaxFile = BUFFER_SIZE;
		ofn.lpstrFileTitle = filename;
		ofn.nMaxFileTitle = BUFFER_SIZE;
		ofn.Flags =
		(opts.allow_multiple ? OFN_ALLOWMULTISELECT : 0) ||
		(opts.prompt_for_create ? OFN_CREATEPROMPT : 0) ||
		(opts.allow_redirect_link ? 0 : OFN_NODEREFERENCELINKS);

		OpenFileResult result;

		if (GetOpenFileName(&ofn)) {
			result.succeded = true;

			// To make sure they are generic.
			result.filename = std::filesystem::path{ filename };
			result.filepath = std::filesystem::path{ filepath };
		}
		else {
			result.succeded = false;
			result.error_code = CommDlgExtendedError();
		}

		callback(result);
	});
	thread.detach();
}


file::OpenFileResult file::open_file(file::OpenFileOpts opts) noexcept {
	constexpr auto BUFFER_SIZE = 512;
    
	char* filepath = new char[BUFFER_SIZE];
	memcpy(
		filepath,
		opts.filepath.string().c_str(),
		opts.filepath.string().size() + 1
	);
	defer{ delete[] filepath; };

	char* filename = new char[BUFFER_SIZE];
	memcpy(
		filename,
		opts.filename.string().c_str(),
		opts.filename.string().size() + 1
	);
	defer{ delete[] filename; };

	const char* filters = create_cstr_extension_label_map(opts.ext_filters);
	defer{ delete[] filters; };

	OPENFILENAMEA ofn;
	ZeroMemory(&ofn, sizeof(ofn));

	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = (HWND)opts.owner;
	ofn.lpstrFilter = filters;
	ofn.lpstrFile = filepath;
	ofn.nMaxFile = BUFFER_SIZE;
	ofn.lpstrFileTitle = filename;
	ofn.nMaxFileTitle = BUFFER_SIZE;
	ofn.Flags =
		(opts.allow_multiple ? OFN_ALLOWMULTISELECT : 0) ||
		(opts.prompt_for_create ? OFN_CREATEPROMPT : 0) ||
		(opts.allow_redirect_link ? 0 : OFN_NODEREFERENCELINKS);

	OpenFileResult result;

	if (GetOpenFileNameA(&ofn)) {
		result.succeded = true;

		// To make sure they are generic.
		result.filename = std::filesystem::path{ filename };
		result.filepath = std::filesystem::path{ filepath };
	}
	else {
		result.succeded = false;
		result.error_code = CommDlgExtendedError();
	}
	return result;
}


void file::open_dir_async(
std::function<void(std::optional<std::filesystem::path>)>&& callback
) noexcept {
	std::thread([callback]() {callback(open_dir()); }).detach();
}
std::optional<std::filesystem::path> file::open_dir() noexcept {
	std::optional<std::filesystem::path> result = std::nullopt;
	std::thread{ [&result] {
		HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
		if (FAILED(hr)) return;

		IFileDialog* file_dialog;
		auto return_code = CoCreateInstance(
			CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&file_dialog)
		);
		if (FAILED(return_code)) return;
		defer{ file_dialog->Release(); };

		DWORD options;
		if (FAILED(file_dialog->GetOptions(&options))) return;

		file_dialog->SetOptions(options | FOS_PICKFOLDERS);

		if (FAILED(file_dialog->Show(NULL))) return;

		IShellItem* psi;
		if (FAILED(file_dialog->GetResult(&psi))) return;
		defer{ psi->Release(); };

		LPWSTR pointer;
		if (FAILED(psi->GetDisplayName(SIGDN_DESKTOPABSOLUTEPARSING, &pointer))) return;
		assert(pointer);

		result = std::filesystem::path{ std::wstring{pointer} };
	} }.join();

	return result;
}

void file::monitor_file(std::filesystem::path path, std::function<void()> f) noexcept {
	file::monitor_dir(path.parent_path(), [path, f](std::filesystem::path changed) {
		if (changed.filename() != path.filename()) return;
		f();
	});
}

void file::monitor_dir(
	std::filesystem::path dir, std::function<void(std::filesystem::path)> f
) noexcept {
	monitor_dir([] {}, dir, f);
}

void file::monitor_dir(
	std::function<void()> init_thread,
	std::filesystem::path dir,
	std::function<void(std::filesystem::path)> f
) noexcept {
	std::thread{ [f, dir, init_thread] {
			init_thread();
			auto handle = CreateFile(
				dir.string().c_str(),
				GENERIC_READ,
				FILE_SHARE_READ | FILE_SHARE_DELETE | FILE_SHARE_WRITE,
				NULL,
				OPEN_EXISTING,
				FILE_FLAG_BACKUP_SEMANTICS,
				NULL
			);
			DWORD buffer[1024];
			DWORD byte_returned;

			while (true) {
				auto result = ReadDirectoryChangesW(
					handle,
					buffer,
					sizeof(buffer),
					TRUE,
					FILE_NOTIFY_CHANGE_LAST_WRITE,
					&byte_returned,
					NULL,
					NULL
				);

				if (!result) {
					// >TODO: error
					continue;
				}


				for (size_t i = 0; i < byte_returned;) {
					auto* info = (FILE_NOTIFY_INFORMATION*)(buffer + i);
					if (info->Action == FILE_ACTION_MODIFIED) {
						using namespace std::chrono_literals;
						// So WinApi doesnt have a function to wait for a file to be free somehow ?
						// When we receive this notification another process just finished writing
						// to this file so it's highly probable that the handle is not free
						// We wait 5ms in the hope to be able to CreateFile.
						std::this_thread::sleep_for(5ms);
						f(std::wstring(info->FileName, info->FileNameLength / sizeof(WCHAR)));
					}

					i += info->NextEntryOffset;
					if (info->NextEntryOffset == 0) break;
				}
			}
		}
	}.detach();
}
