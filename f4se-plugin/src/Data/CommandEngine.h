#pragma once

namespace Data
{
	class CommandEngine
	{
	public:
		struct Value
		{
			void operator=(const std::string& other)
			{
				impl = other;
			}
			void operator=(bool other)
			{
				impl = other;
			}
			void operator=(int other)
			{
				impl = other;
			}
			void operator=(float other)
			{
				impl = other;
			}

			template <typename T>
			std::optional<T> Is() const
			{
				if (const auto v = std::get_if<T>(&impl); v != nullptr) {
					return *v;
				} else {
					return std::nullopt;
				}
			}
		protected:
			std::variant<std::string, bool, int, float> impl = 0i32;
		};

		enum OpType
		{
			kNoOp,
			kJumpIfTrue,
			kJumpIfFalse,
			kSelectActor,
			kForEachActor,
			kApplyMorph
		};

		struct StateMachine
		{
			struct CompiledOp
			{
				std::function<void(StateMachine&, const std::vector<Value>&)> execFunc;
				std::vector<Value> args;
			};

			size_t execIndex = 0;
			size_t execCount = 0;
			std::unordered_map<std::string, size_t> jumpPoints;
			std::vector<CompiledOp> ops;

			int storedIntA = 0;
			std::vector<RE::NiPointer<RE::Actor>> actors;
			size_t selectedActor = 0;

			bool Execute(size_t execLimit = 2000)
			{
				while (execIndex < ops.size() && execCount < execLimit) {
					auto& currentOp = ops[execIndex];
					currentOp.execFunc(*this, currentOp.args);
					execIndex++;
					execCount++;
				}
				return execCount < execLimit;
			}

			bool TryJumpToPoint(const std::string& point)
			{
				auto iter = jumpPoints.find(point);
				if (iter != jumpPoints.end() && iter->second < ops.size()) {
					execIndex = iter->second;
					return true;
				}
				return false;
			}
		};

	protected:
		inline static void NoOp(StateMachine&, const std::vector<Value>&) {}

		inline static void JumpIfTrue(StateMachine& state, const std::vector<Value>& args) {
			if (state.storedIntA > 0 && args.size() >= 1) {
				auto s = args[0].Is<std::string>();
				if (s.has_value()) {
					state.TryJumpToPoint(s.value());
				}
			}
		}

		inline static void JumpIfFalse(StateMachine& state, const std::vector<Value>& args) {
			if (state.storedIntA < 1 && args.size() >= 1) {
				auto s = args[0].Is<std::string>();
				if (s.has_value()) {
					state.TryJumpToPoint(s.value());
				}
			}
		}

		inline static void SelectActor(StateMachine& state, const std::vector<Value>& args) {
			if (args.size() >= 1) {
				auto i = args[0].Is<int>();
				if (i.has_value() && i.value() < state.actors.size()) {

				}
			}
		}

		inline static void ApplyMorph(StateMachine&, const std::vector<Value>&) {

		}
	};
}
