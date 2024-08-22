#ifndef USER_MAPPER_INTERFACE
#define USER_MAPPER_INTERFACE

struct UserMgr;

struct UserPrf {
  char* irods_username;
  char* attributes;
};

extern "C" {
  UserMgr* user_manager_init(const char* _args);
  UserPrf* user_manager_match(UserMgr* _manager, const char* _param);
  void user_manager_close(UserMgr* _manager);
}

#endif
