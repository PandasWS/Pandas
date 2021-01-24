// Copyright(c) Pandas Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#include "aura.hpp"

#include "battle.hpp"
#include "map.hpp"
#include "pc.hpp"

AuraDatabase aura_db;

std::unordered_map<uint16, enum e_aura_special> special_effects{
	{202, AURA_SPECIAL_HIDE_DISAPPEAR},
	{362, AURA_SPECIAL_HIDE_DISAPPEAR}
};

//************************************
// Method:      getDefaultLocation
// Description: 获取 YAML 数据文件的具体路径
// Access:      public 
// Returns:     const std::string
// Author:      Sola丶小克(CairoLee)  2020/09/26 15:30
//************************************
const std::string AuraDatabase::getDefaultLocation() {
	return std::string(db_path) + "/aura_db.yml";
}

//************************************
// Method:      parseBodyNode
// Description: 解析 Body 节点的主要处理函数
// Access:      public 
// Parameter:   const YAML::Node & node
// Returns:     uint64
// Author:      Sola丶小克(CairoLee)  2020/09/26 15:30
//************************************
uint64 AuraDatabase::parseBodyNode(const YAML::Node& node) {
	uint32 aura_id = 0;

	if (!this->asUInt32(node, "AuraID", aura_id)) {
		return 0;
	}

	if (aura_id <= 0) {
		return 0;
	}

	auto aura = this->find(aura_id);
	bool exists = aura != nullptr;

	if (!exists) {
		aura = std::make_shared<s_aura>();
		aura->aura_id = aura_id;
	}
	else {
		aura->effects.clear();
	}

	if (!this->nodeExists(node, "EffectList")) {
		return 0;
	}

	for (const YAML::Node& subNode : node["EffectList"]) {
		uint16 effect_id;

		if (!this->asUInt16(subNode, "EffectID", effect_id)) {
			return 0;
		}

		aura->effects.push_back(effect_id);
	}

	if (!exists) {
		this->put(aura->aura_id, aura);
	}

	return 1;
}

//************************************
// Method:      aura_search
// Description: 根据光环编号获取 aura_db.yml 记录的信息
// Access:      public 
// Parameter:   uint32 aura_id
// Returns:     std::shared_ptr<s_aura>
// Author:      Sola丶小克(CairoLee)  2020/09/26 16:30
//************************************
std::shared_ptr<s_aura> aura_search(uint32 aura_id) {
	return aura_db.find(aura_id);
}

//************************************
// Method:      aura_special
// Description: 给定一个效果编号, 查询它是否是一个特殊效果, 并返回它的标记位
// Access:      public 
// Parameter:   uint16 effect_id
// Returns:     enum e_aura_special 特殊标记位
// Author:      Sola丶小克(CairoLee)  2020/10/08 10:59
//************************************
enum e_aura_special aura_special(uint16 effect_id) {
	if (special_effects.find(effect_id) != special_effects.end()) {
		return special_effects[effect_id];
	}
	return AURA_SPECIAL_NOTHING;
}

//************************************
// Method:      aura_make_effective
// Description: 为 bl 设置光环并使其能够立刻刷新生效
// Access:      public 
// Parameter:   struct block_list * bl
// Parameter:   uint32 aura_id
// Parameter:   bool pc_saved 若是玩家单位, 那么是否保存此光环到数据库
// Returns:     void
// Author:      Sola丶小克(CairoLee)  2020/10/13 00:21
//************************************
void aura_make_effective(struct block_list* bl, uint32 aura_id, bool pc_saved) {
	if (!bl) return;
	if (aura_id && !aura_search(aura_id)) return;

	map_freeblock_lock();

	struct map_data* mapdata = map_getmapdata(bl->m);
	struct s_unit_common_data* ucd = status_get_ucd(bl);

	if (ucd) {
		ucd->aura.id = aura_id;
	}

	switch (bl->type)
	{
	case BL_PC: {
		struct map_session_data* sd = BL_CAST(BL_PC, bl);
		if (!sd) break;

		if (pc_saved) {
			pc_setglobalreg(sd, add_str(AURA_VARIABLE), aura_id);
		}

		if (ucd) {
			ucd->aura.hidden = false;
		}
		pc_setpos(sd, sd->mapindex, sd->bl.x, sd->bl.y, CLR_OUTSIGHT);
		break;
	}
	case BL_MOB:
		// 若是魔物的话, 可以用 clif_clearunit_area 直接发 CLR_TRICKDEAD 清理缓存
		// 而不是和 else 分支一样广播 clif_outsight 封包
		// 这样魔物移动过程中发生光环替换的时候, 才不会有明显的消失后再出现的效果
		if (mapdata && mapdata->users) {
			clif_clearunit_area(bl, CLR_TRICKDEAD);
			map_foreachinallrange(clif_insight, bl, AREA_SIZE, BL_PC, bl);
		}
		break;
	default:
		if (mapdata && mapdata->users) {
			map_foreachinallrange(clif_outsight, bl, AREA_SIZE, BL_PC, bl);
			map_foreachinallrange(clif_insight, bl, AREA_SIZE, BL_PC, bl);
		}
		break;
	}

	map_freeblock_unlock();
}

//************************************
// Method:      aura_reload
// Description: 重新载入 aura_db.yml 光环组合数据库
// Access:      public 
// Parameter:   void
// Returns:     void
// Author:      Sola丶小克(CairoLee)  2020/09/26 16:41
//************************************
void aura_reload(void) {
	aura_db.clear();
	aura_db.load();
}

//************************************
// Method:      do_final_aura
// Description: 释放 AuraDatabase 的实例对象 aura_db 所保存的内容
// Access:      public 
// Parameter:   void
// Returns:     void
// Author:      Sola丶小克(CairoLee)  2020/09/26 16:41
//************************************
void do_final_aura(void) {
	aura_db.clear();
}

//************************************
// Method:      do_init_aura
// Description: 初始化光环机制, 从 aura_db.yml 光环组合数据库中载入数据
// Access:      public 
// Parameter:   void
// Returns:     void
// Author:      Sola丶小克(CairoLee)  2020/09/26 16:41
//************************************
void do_init_aura(void) {
	aura_db.load();
}
