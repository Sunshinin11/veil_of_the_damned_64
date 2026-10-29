"""Stage an explicit ROM manifest; never package the conversion cache wholesale."""
from pathlib import Path
import argparse,fnmatch,re,json,shutil,hashlib
from rom_asset_rules import finite_assets
R=Path(__file__).resolve().parents[1]
def manifest():
    makefile=(R/'Makefile').read_text()
    declaration=re.search(r'^src\s*=\s*(.*?)(?=\n\s*\n)',makefile,re.M|re.S)
    if not declaration:raise RuntimeError('Missing compiled source list in Makefile')
    compiled=re.findall(r'src/[A-Za-z0-9_./-]+\.c',declaration.group(1))
    if not compiled:raise RuntimeError('Empty compiled source list')
    code='\n'.join((R/p).read_text(errors='replace') for p in compiled)
    paths=set(re.findall(r'"rom:/([^"\n]+)"',code))
    literals={s for s in paths if '%' not in s}
    finite_templates={'spr_%s%s_d%d_f%d.%s.sprite','spr_%s_d%d_f%d.%s.sprite','spr_%s.rgba16.sprite','scene_%s.rgba16.sprite','scene_%s_f%d.rgba16.sprite','scene_%s_base.rgba16.sprite','scene_%s_motion_f%d.rgba16.sprite'}
    patterns=[re.sub(r'%[-+0-9.*]*[a-zA-Z]','*',s) for s in paths if '%' in s and s not in finite_templates]
    cache=[p for p in (R/'filesystem').iterdir() if p.is_file()]
    # Begin with exact ROM paths. Only add converted assets that match a
    # run-time formatted path; source files are not automatically ROM content.
    names={Path(p).name for p in literals}
    names.update(finite_assets(R,code,{p.name for p in cache}))

    # Audio filenames come from these two fixed tables. Expanding their %s
    # templates to '*' would accidentally include every unused sound in ROM.
    for table in ('music_names','effect_names'):
        match=re.search(r'\b'+table+r'\s*\[[^]]*\]\s*=\s*\{(.*?)\};',code,re.S)
        if match:
            names.update(value+'.wav64' for value in re.findall(r'"([^"]+)"',match.group(1)))
    patterns=[p for p in patterns if p != '*.wav64']
    for p in cache:
        if p.name in names or any(fnmatch.fnmatchcase(p.name,pat) for pat in patterns):
            names.add(p.name)
    # The approved modular Heart uses only its v2 totem; legacy orange/lava
    # placeholders must not re-enter ROM through the broad scene_%s loader.
    retired={p.name for p in cache if (p.name.startswith('scene_heart_') and not p.name.startswith('scene_heart_v2_totem.')) or p.name in {'scene_floor_heart.rgba16.sprite','scene_wall_heart.rgba16.sprite','scene_wall_heart_top.rgba16.sprite'}}
    # Keep approved snow art and shared frozen figures. Only obsolete props and
    # legacy tiled ground/walls are retired; editable PNG masters stay on disk.
    retired.update(n+'.rgba16.sprite' for n in (
        'scene_snow_rock_a','scene_snow_rock_b','scene_snow_cart',
        'scene_snow_bones','scene_snow_corpse','scene_floor_snow',
        'scene_wall_snow','scene_wall_snow_top','scene_floor_ice',
        'scene_wall_ice','scene_wall_ice_top','scene_ice_mammoth','scene_frost_statue'))
    retired.update(p.name for p in cache if p.name.startswith('scene_floor_') or
        (p.name.startswith('scene_wall_') and p.name!='scene_wall_chains.rgba16.sprite'))
    assert not (retired & literals),f'Retired asset referenced directly: {retired & literals}'
    names.difference_update(retired)
    for model in list(names):
        if model.endswith('.t3dm'):
            names.update(p.name for p in (R/'filesystem').glob(model[:-5]+'.*.sdata'))
    missing=[n for n in names if not (R/'filesystem'/n).is_file()]
    if missing:raise RuntimeError(f'Missing runtime asset: {missing[:20]}')
    lost=sorted(n for n in literals if (R/'filesystem'/n).is_file() and n not in names)
    assert not lost,lost
    return names,paths
def main():
    p=argparse.ArgumentParser();p.add_argument('--output',required=True);p.add_argument('--report',required=True);a=p.parse_args()
    out=Path(a.output).resolve()
    assert R in out.parents and out.name=='romfs' and out!=R/'filesystem',out
    out.mkdir(parents=True,exist_ok=True)
    names,paths=manifest()
    # Only files in our dedicated flat staging directory are replaced.
    for q in out.iterdir():
        assert q.is_file() and not q.is_symlink(),q
        if q.name not in names:q.unlink()
    for name in sorted(names):
        src=R/'filesystem'/name;dst=out/name
        if not dst.exists() or dst.read_bytes()!=src.read_bytes():shutil.copy2(src,dst)
    excluded=[p.name for p in (R/'filesystem').iterdir() if p.is_file() and p.name not in names]
    report={'files':len(names),'bytes':sum((out/n).stat().st_size for n in names),'excluded':[{'name':n,'bytes':(R/'filesystem'/n).stat().st_size} for n in sorted(excluded)],'runtime_path_patterns':sorted(paths),'assets':{n:{'bytes':(out/n).stat().st_size,'sha256':hashlib.sha256((out/n).read_bytes()).hexdigest()} for n in sorted(names)}}
    Path(a.report).write_text(json.dumps(report,indent=2))
    print(f"ROM manifest: {len(names)} files, {report['bytes']} bytes; {len(excluded)} cache residues excluded")
if __name__=='__main__':main()
