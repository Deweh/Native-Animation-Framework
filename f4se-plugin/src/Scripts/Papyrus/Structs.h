#pragma once

namespace Papyrus::NAF
{
	struct IdPacked
	{
		int32_t id1;
		int32_t id2;
	};

	union SceneConvert
	{
		uint64_t full{ 0 };
		IdPacked id;
	};

	using SceneId = RE::BSScript::structure_wrapper<"NAF", "SceneId">;
	using StartSceneResult = RE::BSScript::structure_wrapper<"NAF", "StartSceneResult">;

	std::optional<uint64_t> UnpackU64(std::vector<int32_t> in, size_t startIndex)
	{
		if (in.size() < startIndex + 2) {
			return std::nullopt;
		}

		SceneConvert con;
		con.id.id1 = in[startIndex];
		con.id.id2 = in[startIndex + 1];
		return con.full;
	}

	void PackU64(uint64_t in, std::vector<RE::BSScript::Variable>& out, size_t startIndex) {
		SceneConvert con;
		con.full = in;
		out[startIndex] = con.id.id1;
		out[startIndex + 1] = con.id.id2;
	}

	SceneId PackSceneId(uint64_t in)
	{
		SceneConvert con;
		con.full = in;
		SceneId sceneOut;
		sceneOut.insert("id1", con.id.id1);
		sceneOut.insert("id2", con.id.id2);

		return sceneOut;
	}

	uint64_t UnpackSceneId(SceneId in)
	{
		auto id1 = in.find<int32_t>("id1", true);
		auto id2 = in.find<int32_t>("id2", true);

		if (!id1.has_value() || !id2.has_value()) {
			return 0;
		}

		SceneConvert con;
		con.id.id1 = id1.value();
		con.id.id2 = id2.value();

		return con.full;
	}

	StartSceneResult PackSceneResult(bool success, uint64_t sceneId = 0) {
		SceneConvert con;
		con.full = sceneId;

		StartSceneResult res;
		res.insert("successful", success);
		res.insert("id1", con.id.id1);
		res.insert("id2", con.id.id2);

		return res;
	}
}
