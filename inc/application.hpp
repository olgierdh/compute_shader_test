#pragma once

#include "application_data.hpp"
#include "sdl.hpp"

struct application
{
  public:
    application()
        : m_data{}
        , m_sdl_context{}
        , m_sdl_window{}
    {
    }

    application( const application& ) = delete;
    application& operator=( const application& ) = delete;
    application( application&& )                 = delete;
    application& operator=( application&& ) = delete;

  public:
    application_data& get_data() { return m_data; }
    void initialize();
    void run();

  private:
    application_data m_data;
    sdl_context m_sdl_context;
    sdl_window m_sdl_window;
};
