#ifndef TUPLE_BATCH_NYYMCL31
#define TUPLE_BATCH_NYYMCL31

#include "cube.h"
#include <vector>
#include <list>

namespace jetstream {
namespace cube {

class TupleBatch {

public:
  size_t static const INVALID_POSITION;

  TupleBatch(size_t batch): batch(batch) {};
  virtual ~TupleBatch () {};

  size_t 
    insert_tuple(boost::shared_ptr<jetstream::Tuple> t, bool batch, bool need_new_value, bool need_old_value);
  size_t 
    update_batched_tuple(size_t pos, boost::shared_ptr<jetstream::Tuple> t, bool batch, bool need_new_value, bool need_old_value);

  void flush();
private:

  void save_tuple(boost::shared_ptr<jetstream::Tuple> t, bool need_new_value, bool need_old_value);
  size_t batch_add(boost::shared_ptr<jetstream::Tuple> t, bool need_new_value, bool need_old_value);
  size_t batch_set(boost::shared_ptr<jetstream::Tuple> t, bool need_new_value, bool need_old_value, size_t pos);

  boost::shared_ptr<jetstream::Tuple> get_stored_tuple(size_t pos);
  boost::shared_ptr<jetstream::Tuple> remove_tuple(size_t pos);


  boost::shared_ptr<DataCube> get_cube() { return cube; }
  boost::shared_ptr<DataCube> cube;
  size_t batch;
  std::vector<boost::shared_ptr<jetstream::Tuple> > tuple_store;  
  std::vector<bool> need_new_value_store;  
  std::vector<bool> need_old_value_store; 
  std::list<size_t> holes;
};


  
} /* cube */
} /* jetstream */



#endif /* end of include guard: TUPLE_BATCH_NYYMCL31 */
