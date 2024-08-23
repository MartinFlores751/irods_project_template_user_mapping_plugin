#ifndef USER_MAPPER_INTERFACE
#define USER_MAPPER_INTERFACE

extern "C" {
  void* user_manager_init(const char* _args);
  char* user_manager_match(void* _manager, const char* _param);
  void user_manager_close(void* _manager);
}

#endif
