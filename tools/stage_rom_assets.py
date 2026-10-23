"""Stage an explicit ROM manifest; never package the conversion cache wholesale."""
from pathlib import Path
import argparse,fnmatch,re,json,shutil,hashlib
R=Path(__file__).resolve().parents[1]
def manifest():
    names=set()
    for folder,extensions,suffix in [('assets', ['.png'],'.sprite'),('assets/sprites',['.png'],'.sprite'),('assets/scenery',['.png'],'.sprite'),('assets/audio',['.wav','.mp3'],'.wav64'),('assets/models',['.glb'],'.t3dm'),('assets/fonts',['.ttf'],'.font64')]:
        for ext in extensions:
            names.update(p.stem+suffix for p in (R/folder).glob('*'+ext))
    names.update(['cem_opt_pages.ci8','cem_opt_index.bin','cem_painted_palettes.tlut'])
    code='\n'.join(p.read_text(errors='replace') for p in (R/'src').glob('*.c'))
    paths=set(re.findall(r'"rom:/([^"\n]+)"',code))
    literals={s for s in paths if '%' not in s}
    patterns=[re.sub(r'%[-+0-9.*]*[a-zA-Z]','*',s) for s in paths if '%' in s]
    cache=[p for p in (R/'filesystem').iterdir() if p.is_file()]
    # Preserve legacy animation frames that the dynamic fallback loader can request.
    for p in cache:
        if p.name in literals or (p.suffix=='.sprite' and p.name.startswith(('spr_','scene_')) and any(fnmatch.fnmatchcase(p.name,pat) for pat in patterns)):
            names.add(p.name)
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
