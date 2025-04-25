module;

export module sdl;

export import :types;
export import :pipeline;

export namespace ter
{
	class sdl_base
	{
	public:
		sdl_base()
		{
			auto result = SDL_Init(SDL_INIT_VIDEO);
			assert(result and "SDL could not be initialized.");
		}

		~sdl_base()
		{
			SDL_Quit();
		}
	};
}