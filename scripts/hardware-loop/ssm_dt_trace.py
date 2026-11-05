#!/usr/bin/env python3
"""Offline DT-only opaque-record decoder. No device access or root-cause inference."""
import argparse,base64,importlib.util,json,struct
from pathlib import Path
P=Path(__file__).resolve().parent;R=P
while not (R/'scripts/hardware-loop/ssm_ring.py').is_file():R=R.parent
sp=importlib.util.spec_from_file_location('ring',R/'scripts/hardware-loop/ssm_ring.py');ring=importlib.util.module_from_spec(sp);sp.loader.exec_module(ring)
SCHEMA=json.loads((P/'ssm_dt_trace_schema.json').read_text())
LOW=[('C0',0,8),('C4',8,7),('C9',15,5),('C5',20,5),('in_adj',25,1),('added_line',26,1),('toggle_stage',27,2),('effective_IVM',29,1),('parity_C9',30,1),('parity_frame',31,1),('vsync_allow',32,1),('logical_VSYNC_r',33,1),('selected_register',34,5),('R0',39,8),('R4',47,8),('R5',55,8),('R7',63,8),('R8',71,8),('R9',79,8),('PPI_live_input',87,8)]
def fields(v,table):return {name:(v>>bit)&((1<<n)-1) for name,bit,n in table}
def decode(data):
 h=ring.parse_header(data)
 if h.format_version!=2 or h.entries!=64 or h.dropped:raise ValueError('require format2/64slots/zero dropped')
 records=[]
 for i in range(max(0,h.written-h.entries),h.written):
  a,b=struct.unpack_from('<QQ',data,16+16*(i%64))
  records.append((a&65535,(a>>16)|((b>>48)<<48),(b>>16)&0xffffffff,i,b&65535))
 private=[r for r in records if 0xff20<=r[0]<=0xff2f]
 if not private:raise ValueError('DT flush incomplete: no committed status')
 f7=[i for i,r in enumerate(records) if r[0]==0x2f7 and r[3]<private[0][3]]
 if not f7:raise ValueError('ordinary F7 marker not retained before DT flush')
 start=f7[-1]
 if not any(r[0]==0x2f8 for r in records[:start]):raise ValueError('ordinary F8 marker not retained')
 # One DT packet per enable, published only after F7. Retained F8/F7
 # establish that no earlier private prefix displaced ordinary sequences.
 # Ordinary B.low16 counts events; private B.low16 counts physical writes.
 private_seen=0
 for code,payload,tick,index,seq in records:
  is_private=0xff20<=code<=0xff2f
  expected=index if is_private else index-private_seen
  if seq!=expected&65535:raise ValueError('record sequence mismatch: stale or unpublished ring slot')
  private_seen+=is_private
 if not private or private[-1][0]!=0xff2f:raise ValueError('DT flush incomplete: no committed status')
 status=private[-1][1];count=status&255;version=(status>>8)&255;error=(status>>16)&255
 if status>>32!=0x44545231 or version!=1 or count!=7:raise ValueError('not DT DTR1/version1/sevenslots')
 if len(private)!=15:raise ValueError('missing/duplicate DT record')
 slots=[]
 for i in range(7):
  lo,hi=private[2*i:2*i+2]
  if lo[0]!=0xff20+i or hi[0]!=0xff28+i or lo[2]!=hi[2]:raise ValueError('unordered tags/timestamp mismatch')
  slots.append((lo[1]|(hi[1]<<64),lo[2]))
 summary=fields(slots[6][0],SCHEMA['summary'])
 if summary['reserved'] or summary['error']!=error:raise ValueError('DT summary/status inconsistency')
 mask=summary['slot_valid_mask'];samples=[]
 for i,(v,t) in enumerate(slots[:6]):
  d=fields(v,LOW+SCHEMA['sample_upper_fields']);valid=bool(mask&(1<<i))
  if d['reserved']:raise ValueError('nonzero DT reserved snapshot bits')
  d.update(slot=i,valid=valid,tick64MHz=t,kind='M1 prefix completion' if i<3 else 'actual rawVS rising edge',PC=['69F1','69FC','6A07'][i] if i<3 else None,raw_edge_ordinal=i-2 if i>=3 else None)
  d['published_VSYNC_PB0']=d['PPI_live_input']&1;samples.append(d)
 owners=[]
 for i in range(3):owners.append({'ordinal':i+1,'snapshot_valid':bool(mask&(1<<(3+i))),'actual_C4':samples[3+i]['C4'] if samples[3+i]['valid'] else None,'actual_R7':samples[3+i]['R7'] if samples[3+i]['valid'] else None,'active_black_masterticks':summary[f'owner{i}_active_black_ticks'],'GA_blank_seen':bool(summary['owner_GA_blank_seen']&(1<<i)),'DE_during_GA_blank_seen':bool(summary['owner_DE_seen']&(1<<i)),'active_black_overflow':bool(summary['owner_active_black_overflow']&(1<<i))})
 elapsed_delta=((slots[6][1]-slots[0][1])&0xffffffff) if summary['bracket_started'] else None
 if summary['bracket_complete_tail_validated'] and not summary['elapsed_overflow'] and elapsed_delta!=summary['elapsed_master_ticks']:raise ValueError('bracket timestamp/elapsed mismatch')
 return {'schema':'DTR1/version1','header':h.as_dict(),'status_error':error,'source_qualified_complete_bracket':error==0 and summary['bracket_complete_tail_validated']==1 and mask&7==7,'all_six_slots_present':mask==63,'summary_tick64MHz':slots[6][1],'summary':summary,'samples':samples,'actual_edge_owners':owners,'units':'64MHz masterticks; halfopen firstqualified69F1 completion to next; timestampsmod2^32; no instruction-start/accepted-OUT phase assertion','limits':'Missing edge slots are ordinal, never assigned row14/30. Active black at motherboard boundary alone does not identify an HDMI/scaler root cause. No per-owner GAblank duration is exported. For an incomplete closing tail, the elapsed counter endpoint is not qualified and need not equal the F7 summary timestamp.'}
def main():
 a=argparse.ArgumentParser(description=__doc__);a.add_argument('dump',type=Path,nargs='?');a.add_argument('--base64',action='store_true');a.add_argument('--read-command',action='store_true');x=a.parse_args()
 if x.read_command:print(ring.read_command());return
 if not x.dump:a.error('dump required')
 data=x.dump.read_bytes();data=base64.b64decode(data) if x.base64 else data;print(json.dumps(decode(data),indent=2))
if __name__=='__main__':main()
