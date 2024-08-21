#include "main.hpp"

#include <fstream>

#include <boost/dll.hpp>

LocalFileUserManager::LocalFileUserManager(const nlohmann::json& _config) {
	const auto path{_config.find("file_path")};
	if (path == std::end(_config)) {
		// handle bad case
		irods::http::log::error("[{}]: Unable to find 'file_path' in provided config.", __func__);
	}

	// Save file string
	file_path = *path;

	// Parse into json object
	irods::http::log::trace("[{}]: openning file_path [{}].", __func__, file_path);
	update();
}

	// Return true if did an update?
	// Perhaps we need more info?
	//
	// What if we treat this as a game loop?
	auto update() -> bool {
		try {
			auto json_data{nlohmann::json::parse(std::ifstream{file_path})}; // TODO: Better file handling...

			// Process into list:
			auto base_itter{json_data.items()};
			std::transform(std::begin(base_itter), std::end(base_itter), std::back_inserter(profile_list), [](const auto& _itter) -> UserProfile {
				return {.irods_user_name = _itter.key(), .attributes = _itter.value()};
			});
		} catch(...) {
			irods::http::log::debug("[{}]: There was an exception ...", __func__);
			return false;
		}

		// TODO: REMOVE ME!!!
		std::for_each(std::cbegin(profile_list), std::cend(profile_list), [] (const auto& _profile) -> void {
			irods::http::log::debug("[{}]: has user [{}]", __func__, _profile.irods_user_name);
		});
		return true;
	}

	auto match(const nlohmann::json& _params) -> std::vector<UserProfile> override {
		update(); // in call determine if update needed...
		// Storage for result
		std::vector<UserProfile> res;

		// Remove all items that don't match
		std::remove_copy_if(std::cbegin(profile_list), std::cend(profile_list), std::back_inserter(res), [&_params](const auto& _profile) -> bool {
			auto param_iter{_params.items()};
			// Match all _params to _profile
			// If all desired match criterea exist, then we provide the desired user.
			return std::all_of(std::begin(param_iter), std::end(param_iter), [&_profile](const auto& _item) -> bool {
				if (const auto value_of_interest{_profile.attributes.find(_item.key())}; value_of_interest != std::end(_profile.attributes)) {
					return *value_of_interest == _item.value();
				}
				return false;
			});
		});

		// Provide results
		return res;
	}
private:
	std::string file_path;
	std::vector<UserProfile> profile_list; // We're going to want to lock this when it gets accessed (especially for update case)
};

extern "C" {
	UserMgr* user_manager_init(const char* _args) {
		try {
			irods::http::log::debug("[{}]: recieved _args [{}]", __func__, _args);
			return reinterpret_cast<UserMgr*>(new LocalFileUserManager{nlohmann::json::parse(_args)});
		} catch (...) {
			auto e{std::current_exception()};
			irods::http::log::debug("[{}]: There was an exception ...", __func__);
			std::rethrow_exception(e);
			return nullptr;
		}
	}
	UserPrf* user_manager_match(UserMgr* _manager, const char* _param) {
		try {
			auto res{reinterpret_cast<UserManager*>(_manager)->match(_param)};
			// TODO: Do conversion to Appropriate DS here.
			// Also, figure out how mem management works here?

			// Good nuff, just loggit
			std::for_each(std::cbegin(res), std::cend(res), [] (const auto& _match) -> void {
				irods::http::log::debug("[{}]: matched user [{}]", __func__, _match.irods_user_name);
			});

		} catch (...) {
			auto e{std::current_exception()};
			irods::http::log::debug("[{}]: There was an exception ...", __func__);
			std::rethrow_exception(e);
		}
		return nullptr;
	}
	void user_manager_close(UserMgr* _manager) {
		irods::http::log::debug("[{}]: recieved _manager [{}]. Cleaning up.", __func__, fmt::ptr(_manager));
		delete reinterpret_cast<UserManager*>(_manager);
		_manager = nullptr;
	}
}

