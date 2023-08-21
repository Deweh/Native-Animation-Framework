/*
MIT License

Copyright (c) 2022 shad0wshayd3

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#pragma once


namespace Papyrus
{
	namespace detail
	{
		template <class... Args>
		RE::BSScrapArray<RE::BSScript::Variable>
			PackVariables(Args&&... a_args)
		{
			constexpr auto size = sizeof...(a_args);
			auto args = std::make_tuple(std::forward<Args>(a_args)...);
			RE::BSScrapArray<RE::BSScript::Variable> result{ size };
			[&]<std::size_t... p>(std::index_sequence<p...>)
			{
				((RE::BSScript::PackVariable(result.at(p), std::get<p>(args))), ...);
			}
			(std::make_index_sequence<size>{});
			return result;
		}

		class FunctionArgsBase
		{
		public:
			FunctionArgsBase() = delete;

			FunctionArgsBase(RE::BSScript::IVirtualMachine* a_vm) :
				vm(a_vm)
			{}

			bool operator()(RE::BSScrapArray<RE::BSScript::Variable>& a_args)
			{
				args->GetArrayCopy(a_args, *vm);
				return true;
			}

		protected:
			// members
			RE::BSScript::ArrayWrapper<RE::BSScript::Variable>* args;  // 00
			RE::BSScript::IVirtualMachine* vm;                         // 08
		};

		static_assert(sizeof(FunctionArgsBase) == 0x10);

		inline RE::BSTThreadScrapFunction<bool(RE::BSScrapArray<RE::BSScript::Variable>&)>
			CreateThreadScrapFunction(FunctionArgsBase& a_args)
		{
			using func_t = decltype(&detail::CreateThreadScrapFunction);
			REL::Relocation<func_t> func{ REL::ID(69733) };
			return func(a_args);
		}
	}

	template <class... Args>
	class FunctionArgs :
		public detail::FunctionArgsBase
	{
	public:
		FunctionArgs() = delete;

		FunctionArgs(RE::BSScript::IVirtualMachine* a_vm, Args... a_args) :
			FunctionArgsBase(a_vm)
		{
			auto scrap = detail::PackVariables(a_args...);
			args = new RE::BSScript::ArrayWrapper<RE::BSScript::Variable>(scrap, *vm);
		}

		RE::BSTThreadScrapFunction<bool(RE::BSScrapArray<RE::BSScript::Variable>&)> get()
		{
			return detail::CreateThreadScrapFunction(*this);
		}
	};

	static_assert(sizeof(FunctionArgs<std::monostate>) == 0x10);
}

