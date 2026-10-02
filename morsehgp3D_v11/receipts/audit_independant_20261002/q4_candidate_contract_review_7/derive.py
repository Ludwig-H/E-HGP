from pathlib import Path
import json,platform,sys
from trace import model
R=Path(__file__).resolve().parent
(R/'TRACE.json').write_text(json.dumps(model(),indent=2,sort_keys=True)+'\n')
(R/'ENVIRONMENT.json').write_text(json.dumps({'python':sys.version,'platform':platform.platform(),'scope':'Python receipt tooling and two tiny analytic fixtures only'},indent=2)+'\n')
print(json.dumps({'fixtures':2,'missing_inside_mutant_emitted':model()['missing_inside_mutant']['emitted'],'native_executed':False},sort_keys=True))
