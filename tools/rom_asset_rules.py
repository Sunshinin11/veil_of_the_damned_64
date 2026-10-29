"""Finite expansion of runtime asset tables; no broad spr_* or scene_* glob.

Mirror frame_sprite's preferred format and per-frame smooth -> original fallback.
Keep all eight directions and all sixteen allowed frame arguments conservatively.
Presence probes remain rooted by the main manifest's exact literals.
"""
import re,json

def table(code,name):
    m=re.search(r'\b'+name+r'\s*\[[^]]*\]\s*=\s*\{(.*?)\};',code,re.S)
    if not m:raise RuntimeError('Missing runtime asset table '+name)
    return re.findall(r'"([^"\n]+)"',m.group(1))

def animation_assets(root,code,available):
    sprite=(root/'src/sprites.c').read_text()
    match=re.search(r'typedef enum\s*\{(.*?)\}\s*Anim;',sprite,re.S)
    body=re.sub(r'//[^\n]*|/\*.*?\*/','',match.group(1),flags=re.S)
    ids=[t.strip() for t in body.split(',') if t.strip() and t.strip()!='ANIM_COUNT']
    names=table(sprite,'names')
    if len(ids)!=len(names):raise RuntimeError('Animation table/enumeration mismatch')
    index={v:i for i,v in enumerate(ids)}
    nf=int(re.search(r'#define\s+NF_PLAYER\s+(\d+)',(root/'include/anim_frames.h').read_text()).group(1))
    frames=(root/'include/anim_frames.h').read_text()
    result=set()
    approved=json.loads((root/'assets/approved_campaign_art_manifest.json').read_text())['animations']
    heroes=json.loads((root/'assets/approved_hero_art_manifest.json').read_text())['animations']
    smooth_expr=re.search(r'static bool smooth_anim\(Anim a\)\s*\{\s*return (.*?);',sprite,re.S).group(1)
    smooth_expr=re.sub(r'\b[A-Z][A-Z0-9_]*\b',lambda m:str(index[m.group()]),smooth_expr)
    smooth_expr=smooth_expr.replace('&&',' and ').replace('||',' or ')
    if re.sub(r'\b(?:a|and|or)\b|[0-9\s()<>=!]+','',smooth_expr):
        raise RuntimeError('Unsupported smooth_anim expression; update ROM resolver')
    for i,(anim,name) in enumerate(zip(ids,names)):
        if anim.startswith('HA_'):
            spec=heroes[name]
            if spec['active']:
                for direction in spec['directions']:
                    for frame in range(spec['frames']):
                        path=f'spr_{name}_d{direction}_f{frame}.rgba16.sprite'
                        if path in available:result.add(path)
            continue
        if anim.startswith('DW_'):
            directions={'DW_CONTACT':[0,1,2],'DW_WALK':[0,1,2],'DW_CAST':[0,1,2],'DW_DIE':[0]}[anim]
            for direction in directions:
                for frame in range(1 if anim=='DW_CONTACT' else 4):
                    path=f'spr_{name}_d{direction}_f{frame}.rgba16.sprite'
                    if path in available:result.add(path)
            continue
        if anim.startswith('APV_'):
            spec=approved[name]
            for direction in spec['directions']:
                for frame in range(spec['frames']):
                    path=f'spr_{name}_d{direction}_f{frame}.rgba16.sprite'
                    if path in available:result.add(path)
            continue
        # These complete five-view actions now resolve exclusively to accepted
        # HA art. Remove their superseded source sheets from cartridge selection.
        if anim in {'DR_ATTACK','BE_ATTACK','DR_ESCAPE','BE_ESCAPE','DR_TRANSFORM','HU_ATTACK','HU_ROLL'}:continue
        # Human movement now always selects the approved eight-view video run.
        if anim in {'DR_WALK','DR_WALK_ALT','DR_RUN','DR_RUN_ALT','DR_MOVE_SIDE','DR_MOVE_FRONT','DR_MOVE_BACK','DR_NRUN_S','DR_NRUN_N','DR_NRUN_W','DR_NRUN_SW'}:continue
        # Malakor's live basic/slam/pull paths use MH_PUNCH/GROUND/PULL.
        if anim in {'MH_ATTACK','MH_ENRAGE'}:continue
        # All four legacy forest/northern boss families are fully replaced,
        # including contact fallback. Shared Cemetery families remain live.
        if anim.startswith(('SY_','RM_','JA_','DG_')):continue
        # Normal mobs rest on their first walk frame; bosses keep real idles.
        if anim in {'SK_IDLE','DE_IDLE','CU_IDLE'}:continue
        if anim in {'VK_ATTACK','VK_HURT','VK_DEATH','VK_ENRAGE'}:continue # remapped to v4 before emit
        if anim=='MH_DEATH':continue # every boss dies in its second form
        # MA_* are abstract boss states; add_enemies always remaps them to
        # campaign-specific artwork before emit. Legacy generic sheets are dead.
        if index['MA_IDLE']<=i<=index['MA_ENRAGE']:continue
        smooth=eval(smooth_expr,{'__builtins__':{}},{'a':i})
        fmt='rgba32' if index['WO_IDLE']<=i<=index['WO_SUMMON'] or anim in {'BA_LEAP','BA_WHIRLWIND'} else 'rgba16'
        count=nf if smooth else 4
        if anim in {'DR_MOVE_SIDE','DR_MOVE_FRONT','DR_MOVE_BACK'}:
            smooth=False # matches smooth_anim: these are outside the druid range
            count=int(re.search(r'#define\s+NF_'+anim+r'\s+(\d+)',frames).group(1))
        # Only three stored views; W shares E through the runtime flip flag.
        cardinal_enemy=anim in {'SK_WALK','SK_ATTACK','DE_WALK','DE_ATTACK','DE_SLAM','CU_WALK','CU_ATTACK','CU_DEATH','MH_IDLE','MH_WALK','VK_IDLE','VK_WALK','MH_PUNCH','MH_GROUND','VK_SCYTHE'}
        directions=3 if cardinal_enemy else 8
        stored_frames=4 if cardinal_enemy else 16
        if anim in {'MH_IDLE','VK_IDLE'}:stored_frames=1
        if anim=='DE_DEATH':
            directions=4
            stored_frames=2 # Only the two preserved death poses, no damage reaction.
        if anim=='CU_DEATH':stored_frames=2
        if anim in {'FX_CULTIST_SKULL','FX_CULTIST_HEAL','CHEST','CHEST_STEEL','CHEST_QUEST','MH_PULL','FX_MH_WAVE','VK_DROP','VK_MORPH','VK_DIE','FX_VK_IMPACT'}:
            directions=1
            stored_frames=4
        selected_frames=range(stored_frames)
        if anim=='SK_HURT':
            directions=4;selected_frames=(1,2,3) # hit reaction and preserved death only
        if anim=='MH_HURT':
            directions=4;selected_frames=(1,) # living first-form reaction only
        for direction in range(directions):
            for frame in selected_frames:
                preferred=f'spr_{name}{"_s" if smooth else ""}_d{direction}_f{frame}.{fmt}.sprite'
                if preferred in available:result.add(preferred)
                elif smooth:
                    fallback=f'spr_{name}_d{direction}_f{frame*4//count}.{fmt}.sprite'
                    if fallback in available:result.add(fallback)
    result.update('spr_'+n+'.rgba16.sprite' for n in table(sprite,'ui_names'))
    return result

def scenery_assets(root,available):
    # Include existing static AND 0..3 animated forms for every scene table name;
    # this conservatively supports scene_is_animated without guessing placements.
    code=(root/'src/sprites.c').read_text()
    result=set()
    for name in table(code,'scene_names'):
        if name in {'ex_desert_brazier','ex_vict_quarantine'}:
            candidates=[f'scene_{name}_base.rgba16.sprite']+[f'scene_{name}_motion_f{f}.rgba16.sprite' for f in range(4)]
        else:
            candidates=[f'scene_{name}.rgba16.sprite']+[f'scene_{name}_f{f}.rgba16.sprite' for f in range(4)]
        result.update(n for n in candidates if n in available)
    return result

def finite_assets(root,code,available):
    return animation_assets(root,code,available)|scenery_assets(root,available)
