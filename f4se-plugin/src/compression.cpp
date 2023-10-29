
int deflateEnd(z_streamp strm)
{
	using func_t = decltype(&deflateEnd);
	REL::Relocation<func_t> func{ REL::ID(1311509) };
	return func(strm);
}

int inflateEnd(z_streamp strm)
{
	using func_t = decltype(&inflateEnd);
	REL::Relocation<func_t> func{ REL::ID(1144181) };
	return func(strm);
}

int deflateInit2_(z_streamp strm, int level, int method,
	int windowBits, int memLevel,
	int strategy, const char* version,
	int stream_size)
{
	using func_t = decltype(&deflateInit2_);
	REL::Relocation<func_t> func{ REL::ID(1125008) };
	return func(strm, level, method, windowBits, memLevel, strategy, version, stream_size);
}

int inflateInit2_(z_streamp strm, int windowBits,
	const char* version, int stream_size)
{
	using func_t = decltype(&inflateInit2_);
	REL::Relocation<func_t> func{ REL::ID(346092) };
	return func(strm, windowBits, version, stream_size);
}

int deflateReset(z_streamp strm)
{
	using func_t = decltype(&deflateReset);
	REL::Relocation<func_t> func{ REL::ID(1415361) };
	return func(strm);
}

int deflate(z_streamp strm, int flush)
{
	using func_t = decltype(&deflate);
	REL::Relocation<func_t> func{ REL::ID(679101) };
	return func(strm, flush);
}

int inflate(z_streamp strm, int flush)
{
	using func_t = decltype(&inflate);
	REL::Relocation<func_t> func{ REL::ID(224011) };
	return func(strm, flush);
}
