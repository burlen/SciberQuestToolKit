
inline
int min(int a,int b){ return a<b?a:b; }

typedef int Block[2];

class WorkQueue
{
public:
  WorkQueue(int size)
      :
    m_at(0),
    m_end(size)
     { }

  int GetBlock(Block b, int size)
    {
    if (m_at==m_end)
      {
      b[0]=b[1]=0;
      return 0;
      }
    int
    next_at=min(m_at+size,m_end);
    size=end-next_at;
    b[0]=m_at;
    b[1]=size;
    m_at=next_at;
    return size;
    }


private:
  int m_at;
  int m_end;

private:
  WorkQueue();
};
