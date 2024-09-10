#include "main.hpp"

#include <cstdlib>
#include <cstring>
#include <fstream>

#include <boost/dll.hpp>
#include <spdlog/spdlog.h>

LocalFileUserManager::LocalFileUserManager(const nlohmann::json& _config) {
	const auto path{_config.find("file_path")};
	if (path == std::end(_config)) {
		// handle bad case
		spdlog::error("[{}]: Unable to find 'file_path' in provided config.", __func__);
	}

	// Save file string
	file_path = *path;

	// Parse into json object
	spdlog::trace("[{}]: openning file_path [{}].", __func__, file_path);
	update();
}

// Return true if did an update?
// Perhaps we need more info?
//
// What if we treat this as a game loop?
auto LocalFileUserManager::update() -> bool {
	try {
		std::ifstream file{file_path};
		spdlog::debug("[{}]: Opened file [{}]", __func__, file_path);

		auto json_data{nlohmann::json::parse(file)}; // TODO: Better file handling...

		// Create temp list to be swapped in later
		std::vector<UserProfile> temp_list;
		temp_list.reserve(json_data.size());

		// Process into list:
		auto base_itter{json_data.items()};
		std::transform(std::begin(base_itter), std::end(base_itter), std::back_inserter(temp_list), [](const auto& _itter) -> UserProfile {
			return {.irods_user_name = _itter.key(), .attributes = _itter.value()};
		});

		// Move new item list to old one
		profile_list = std::move(temp_list);
	} catch(...) {
		auto e{std::current_exception()};
		spdlog::debug("[{}]: There was an exception ...", __func__);
		std::rethrow_exception(e);
		return false;
	}

	// TODO: REMOVE ME!!!
	std::for_each(std::cbegin(profile_list), std::cend(profile_list), [] (const auto& _profile) -> void {
		spdlog::debug("[{}]: has user [{}]", __func__, _profile.irods_user_name);
	});
	return true;
}

auto LocalFileUserManager::match(const nlohmann::json& _params) -> std::optional<std::string> {
	update(); // in call determine if update needed...
	// Storage for result
	// std::vector<UserProfile> res;

	// Remove all items that don't match
	// std::remove_copy_if(std::cbegin(profile_list), std::cend(profile_list), std::back_inserter(res), [&_params](const auto& _profile) -> bool {
	// 	auto param_iter{_params.items()};
	// 	// Match all _params to _profile
	// 	// If all desired match criterea exist, then we provide the desired user.
	// 	return std::all_of(std::begin(param_iter), std::end(param_iter), [&_profile](const auto& _item) -> bool {
	// 		if (const auto value_of_interest{_profile.attributes.find(_item.key())}; value_of_interest != std::end(_profile.attributes)) {
	// 			return *value_of_interest == _item.value();
	// 		}
	// 		return false;
	// 	});
	// });

	for (const auto& [irods_username, attributes] : profile_list) {
		auto attr_iter{attributes.items()};
		auto res{std::all_of(std::begin(attr_iter), std::end(attr_iter), [&_params](const auto& _iter) -> bool {
			const auto value_of_interest{_params.find(_iter.key())};
			if (value_of_interest == std::end(_params)) {
				return false;
			}

			return *value_of_interest == _iter.value();
		})};

		if (res) {
			return {irods_username};
		}
	}

	// Provide results
	return std::nullopt;
}

extern "C" {
	void* user_manager_init(const char* _args) {
		try {
			spdlog::debug("[{}]: recieved _args [{}]", __func__, _args);
			return reinterpret_cast<void*>(new LocalFileUserManager{nlohmann::json::parse(_args)});
		} catch (...) {
			auto e{std::current_exception()};
			spdlog::debug("[{}]: There was an exception ...", __func__);
			std::rethrow_exception(e);
			return nullptr;
		}
	}
	char* user_manager_match(void* _manager, const char* _param) {
		spdlog::debug("[{}]: Attempting match of _param [{}]", __func__, _param);
		try {
			auto res{reinterpret_cast<UserManager*>(_manager)->match(nlohmann::json::parse(_param))};
			// TODO: Do conversion to Appropriate DS here.
			// Also, figure out how mem management works here?

			// Good nuff, just loggit
			if (res) {
				spdlog::debug("[{}]: Eagerly matched _param [{}] to user [{}]", __func__, _param, *res);
				char* str_thang{static_cast<char*>(std::malloc(res->size()))};
				std::strncpy(str_thang, res->c_str(), res->size() + 1);
				return str_thang;
			}
		} catch (...) {
			auto e{std::current_exception()};
			spdlog::debug("[{}]: There was an exception ...", __func__);
			std::rethrow_exception(e);
		}
		return nullptr;
	}
	void user_manager_close(void* _manager) {
		spdlog::debug("[{}]: recieved _manager [{}]. Cleaning up.", __func__, fmt::ptr(_manager));
		delete reinterpret_cast<UserManager*>(_manager);
		_manager = nullptr;
	}
}

