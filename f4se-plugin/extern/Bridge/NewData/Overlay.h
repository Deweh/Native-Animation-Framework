#pragma once

namespace Data
{
	class Overlay : public IdentifiableObject
	{
	public:
		inline static std::atomic<uint64_t> nextFileId = 0;
		std::string fileName;

		struct OverlayData
		{
			std::string Template;
			int alpha;
			bool isFemale;

			void clear()
			{
				Template.clear();
				alpha = 0;
				isFemale = false;
			}
		};

		float duration;
		int quantity;

		std::vector<OverlayData> overlays;


		static bool Parse(XMLUtil::Mapper& m, Overlay& out)
		{
			out.ParseID(m);
			out.fileName = m.GetFileName();
			
			OverlayData data;
			int depth = 0;
			depth = m.DownNode("condition", "Overlay has no 'condition' node!", true) ? ++depth : depth;
			depth = m.DownNode("overlayGroup", "Overlay has no 'overlayGroup' node!", true) ? ++depth : depth;
			m(&out.duration, 0.f, true, false, "", "duration");
			m(&out.quantity, 1, true, false, "", "quantity");

			//logger::info("defaults node : {}", m.GetDefaultsNode().name());
			//logger::info("defaults loadPriority: {}", *m.GetDefaultsNode().attributes().begin()->name());

			m.GetArray([&](XMLUtil::Mapper& m) {
				data.clear();
				m(&data.Template, XMLUtil::Mapper::emptyStr, true, true, "Overlay has no 'template' attribute!", "template");
				m(&data.alpha, 100, true, false, "", "alpha");
				m(&data.isFemale, true, true, false, "", "isFemale");

				out.overlays.push_back(data);
				return m;
			},
				"overlay", "");

			while (depth)
				m.UpNode(),--depth;

			return m;
		}
	};
}
