#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#ifndef USER_MAPPER_CPP
#define USER_MAPPER_CPP

struct UserProfile {
	std::string irods_user_name; // TODO: Determine if a separate name is actually beneficial
	nlohmann::json attributes;
};

class UserManager {
public:
	virtual ~UserManager() = default;
	virtual auto match(const nlohmann::json& _params) -> std::vector<UserProfile> = 0;
};

// Local is only designed for one process/multi-thread.
//
// Basic idea, load file, get data, refresh if file is touched?
// Since this is read-only, this should work cross platform.
class LocalFileUserManager final : public UserManager {
public:
	explicit LocalFileUserManager(const nlohmann::json& _config);
	auto update() -> bool;
	auto match(const nlohmann::json& _params) -> std::vector<UserProfile> override;
private:
	std::string file_path;
	std::vector<UserProfile> profile_list; // We're going to want to lock this when it gets accessed (especially for update case)
};

#endif
