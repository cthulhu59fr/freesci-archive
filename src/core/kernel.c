#include <script.h>
#include <vm.h>

kernel_function* kfuncs[256]; /* Initialized at run time */


/*Here comes implementations of some kernel functions. I haven't found any
 *documentation on these, except for their names in vocab.999.
 */

#if 0

int kClone(state* s)
{
  instance* old=instance_map[getInt16(s->heap->data+s->acc)];
  instance* new=instantiate(s->heap, old->class);
  int i;
  if(new==0) return 1;

  /*copy the selector values...*/
  for(i=0; i<old->class->selector_count*2; i++)
    new->heap[new->offset+i+2]=old->heap[old->offset+i+2];
  /*...but use the correct id...*/
  putInt16(new->heap+new->offset, new->id);
  /*...mark as clone (ie set bit 0 in -info-)...*/
  putInt16(new->heap+new->offset+6,
	   (getInt16(new->heap+new->offset+6)|1));
  /*...and use old's id as superclass*/
  putInt16(new->heap+new->offset+4, old->id);

  /*set acc to point to the new instance*/
  s->acc=new->offset;
  s->sp-=4;
  return 0;
}

int kDisposeClone(state* s)
{
  int id=getInt16(s->heap->data+s->acc);
  instance* i;
  /*FIXME: check acc for sanity*/

  i=instance_map[id];
  if(h_free(s->heap, i->offset, (i->class->selector_count+1)*2)) return 1;

  /*shuffle the instance_map*/
  instance_map[id]=instance_map[max_instance--];
  putInt16(s->heap->data+instance_map[id]->offset, id);

  s->sp-=4;
  return 0;
}

int kIsObject(state* s)
{
  int id=getInt16(s->heap->data+s->acc);

  if(id<=0x4000 && instance_map[id]->offset==s->acc) s->acc=1;
  else s->acc=0;

  s->sp-=4;
  return 0;
}

#endif /* false */

struct {
  char *functname; /* String representation of the kernel function as in script.999 */
  kernel_function *kfunct; /* The actual function */
} kfunct_mappers[] = {
  {0,0} /* Terminator */
};
