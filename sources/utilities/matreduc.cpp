/*
  matreduc.cpp

  Copyright (C) 2009 Marc van Leeuwen
  part of the Atlas of Reductive Lie Groups

  For license information see the LICENSE file
*/
#include "matreduc.h"

#include "latticetypes_fwd.h"
#include "bitmap.h"
#include "matrix.h"
#include "intutils.h"

namespace atlas {

namespace matreduc {

// make |M(i,j)==0| and |M(i,k)>0| by operations with columns |j| and |k|
// precondition |M(i,k)>0|
template<typename C>
void column_clear(matrix::Matrix<C>& M, size_t i, size_t j, size_t k)
{
  if (M(i,j)<0) // happens often in Cartan matrices; result is more pleasant
    M.columnMultiply(j,-1); // if we ensure non-negativity the easy way
  do
  {
    M.columnOperation(j,k,-(M(i,j)/M(i,k))); // makes |M(i,j)>=0| smaller
    if (M(i,j)==C(0))
      return;
    M.columnOperation(k,j,-(M(i,k)/M(i,j))); // makes |M(i,k)>=0| smaller
  }
  while (M(i,k)!=C(0));
  M.swapColumns(j,k); // only upon "normal" exit of loop swapping is needed
}

// make |M(i,j)==0| and |M(k,j)>0| by operations with rows |i| and |k|
// precondition |M(k,j)>0|
template<typename C>
void row_clear(matrix::Matrix<C>& M, size_t i, size_t j, size_t k)
{
  if (M(i,j)<0)
    M.rowMultiply(i,-1);
  do
  {
    M.rowOperation(i,k,-(M(i,j)/M(k,j))); // makes |M(i,j)>=0| smaller
    if (M(i,j)==C(0))
      return;
    M.rowOperation(k,i,-(M(k,j)/M(i,j))); // makes |M(k,j)>=0| smaller
  }
  while (M(k,j)!=C(0));
  M.swapRows(i,k); // only upon "normal" exit of loop swapping is needed
}

/* Same operations with "recording" matrix |rec| on which same ops are done.
   With the number of operations needed probably quite small on average, the
   cheapest solution is probably to do recording operations immediately.
*/
// make |M(i,j)==0| and |M(i,k)>0| by operations with columns |j| and |k|
// precondition |M(i,k)>0|
template<typename C>
void column_clear(matrix::Matrix<C>& M, size_t i, size_t j, size_t k,
		  matrix::Matrix<C>& rec)
{
  if (M(i,j)<0)
  {
    M.columnMultiply(j,-1);
    rec.columnMultiply(j,-1);
  }
  do
  {
    C q = -(M(i,j)/M(i,k));
    M.columnOperation(j,k,q); // now |M(i,j)>=0|
    rec.columnOperation(j,k,q);
    if (M(i,j)==C(0))
      return;
    q=-(M(i,k)/M(i,j));
    M.columnOperation(k,j,q);
    rec.columnOperation(k,j,q);
  }
  while (M(i,k)!=C(0));
  M.swapColumns(j,k);
  rec.swapColumns(j,k);
}

/* make |M(i,j)==0| and |M(k,j)>0| by operations with rows |i| and |k|
   precondition |M(k,j)>0|. This function has a variant where row operations
   are recorded inversely, in other words by column operations in the opposite
   sense; the template parameter |direct| tells whether recoring is direct
*/
template<typename C, bool direct>
void row_clear(matrix::Matrix<C>& M, size_t i, size_t j, size_t k,
	       matrix::Matrix<C>& rec)
{
  // Rather than forcing the sign of |M(i,j)|, we shall use |intutils::divide|
  // For |direct==false| this often avoids touching column |i| of |rec|
  do
  {
    C q = intutils::divide(M(i,j),M(k,j));
    M.rowOperation(i,k,-q); // now |M(i,j)>=0|, even if |M(i,j)| was negative
    if (direct)
      rec.rowOperation(i,k,-q);
    else // do inverse column operation
      rec.columnOperation(k,i,q);
    if (M(i,j)==C(0))
      return;
    q=M(k,j)/M(i,j);
    M.rowOperation(k,i,-q);
    if (direct)
      rec.rowOperation(k,i,-q);
    else // inverse column operation
      rec.columnOperation(i,k,q);
  }
  while (M(k,j)!=C(0));
  M.swapRows(i,k);
  if (direct)
    rec.swapRows(i,k);
  else
    rec.swapColumns(k,i);
}

/* transform |M| to column echelon form
   postcondition: |M| unchanged column span, |result.size==M.numColumns|, and
   for all |j| and |i=result.n_th(j)|, |M(i,j)>0| and |M(ii,j)==0| for |ii>i|
*/
template<typename C>
  bitmap::BitMap column_echelon(matrix::Matrix<C>& M)
{
  bitmap::BitMap result(M.numRows());
  for (size_t j=M.numColumns(); j-->0;)
  { size_t i=M.numRows();
    while (i-->0)
      if (M(i,j)!=C(0))
      {
	if (result.isMember(i))
	{
	  size_t k=j+1+result.position(i); // index of column with pivot at |i|
	  column_clear(M,i,j,k); // now column |j| is empty in row |i| and below
	}
	else // now column |j| will have pivot in row |i|
	{
          if (M(i,j)<0)
	    M.columnMultiply(j,-1);
	  result.insert(i);
	  size_t p=result.position(i);
	  if (p>0) // then move column |j| to the right |p| places
	  {
	    matrix::Vector<C> c=M.column(j);
	    for (size_t d=j; d<j+p; ++d)
	      M.set_column(d,M.column(d+1));
            M.set_column(j+p,c);
	  }
	  break; // from |while| loop; done with decreasing |i|
	}
      } // |if (M(i,j)!=0)| and |while (i-->0)|
    if (i==size_t(~0)) // no pivot found for column |j|; forget about it
      M.eraseColumn(j); // and no bit is set in |result| for |j| now
  }

  return result;
}

/* find invertible |row|, |col| such that $row*M*col$ is diagonal, and
   return diagonal entries. The result is not unique
*/
template<typename C>
std::vector<C> diagonalise(matrix::Matrix<C> M, // by value
			   matrix::Matrix<C>& row,
			   matrix::Matrix<C>& col)
{
  size_t m=M.numRows();
  size_t n=M.numColumns(); // in fact start of known null columns

  matrix::Matrix<C>(m).swap(row); // intialise |row| to identity matrix
  matrix::Matrix<C>(n).swap(col);
  std::vector<C> diagonal(intutils::min(m,n),C(0));

  for (size_t d=0; d<m and d<n; ++d)
  {
    while (M.column(d).isZero()) // ensure column |d| is nonzero
    {
      --n;
      if (d==n) // then remainder of matrix is zero
	return diagonal;
      M.swapColumns(d,n); // now matrix is zero from column |n| on
      col.swapColumns(d,n);
    }

    { // get nonzero entry from column |d| at (d,d), and make it positive
      size_t i=d;
      while (M(i,d)==C(0)) // guaranteed to terminate
	++i;
      if (i>d)
      {
	C u = M(i,d)>0 ? 1 : -1;
	M.rowOperation(d,i,u);
	row.rowOperation(d,i,u);
      }
      else if (M(d,d)<0)
      {
	M.rowMultiply(d,-1);
	row.rowMultiply(d,-1);
      }
    }

    for (size_t i=d+1; i<m; ++i)
      row_clear<C,true>(M,i,d,d,row); // makes |M(d,d)==gcd>0| and |M(i,d)==0|

    // initial sweep is unlikely to be sufficient, so no termination test here

    size_t i,j=d+1; // these need to survive loops below (but one would suffice)
    do // sweep row and column alternatively with |M(d,d)| until both cleared
    {
     for ( ; j<n; ++j)
       column_clear(M,d,j,d,col); // makes |M(d,d)==gcd>0| and |M(d,j)==0|

     for (i=d+1; i<m; ++i)
       if (M(i,d)!=C(0))
         break;
     if (i==m) // then whole column below |M(d,d)| is zero, and done for |d|
       break; // most likely because |column_clear| left column |d| unchanged

     for ( ; i<m; ++i)
       row_clear<C,true>(M,i,d,d,row); // makes |M(d,d)==gcd>0| and |M(i,d)==0|

     for (j=d+1; j<n; ++j)
       if (M(d,j)!=C(0))
         break;
    }
    while(j<n); // we did |break|, apparently some |row_clear| changed row |d|

    diagonal[d] = M(d,d);
  } // |for d|

  return diagonal;
}

/* The following is a variation of |diagonalise|, used in cases in which we
   are mostly interested in the matrix |B=row.inverse()| and possibly also the
   vector |diagonal|, because they give a transparent expression for the image
   (column span) of |M|: that image is the same as that of $B*D$ where $D$ is
   the diaganal matrix corresponding to |diagonal|, in other words it is
   spanned by the multiples of the columns of $B$ by their |diagonal| factors.

   The procedure follws the mostly the same steps, but the difference in
   handling |row|, where we apply instead of row operations the inverse column
   operations, are sufficiently important to justify the code duplication. We
   take advantage of this duplication to organise the algorithm for minimal
   use of row operations, which besides being more efficient tends to give a
   basis more closely related to the original matrix.

   The name of this function is a slight misnomer, since the basis is not
   necessarily one of a Smith normal form, as |diagonal| is not ordered by
   by divisibility. For most applications this will not be needed anyway.
 */
template<typename C>
matrix::Matrix<C> Smith_basis(matrix::Matrix<C> M, // by value
			      std::vector<C>& diagonal)
{
  size_t m=M.numRows();
  size_t n=M.numColumns(); // in fact start of known null columns

  matrix::Matrix<C> result (m); // intialise |result| to identity matrix
  matrix::Vector<C>(m,C(0)).swap(diagonal);

  for (size_t d=0; d<m and d<n; ++d)
  {
    while (M.column(d).isZero()) // ensure column |d| is nonzero
    {
      --n;
      if (d==n) // then this was the last column, quit
	return result;
      M.eraseColumn(d);
    }

    { // get nonzero entry from column |d| at (d,d), and make it positive
      size_t i=d;
      while (M(i,d)==C(0)) // guaranteed to terminate
	++i;
      if (i>d)
      {
	C u = M(i,d)>0 ? 1 : -1;
	M.rowOperation(d,i,u); // make |M(d,d)==abs(M(i,d))|
	result.columnOperation(i,d,-u); // inverse operation on basis
      }
      else if (M(d,d)<0)
	M.columnMultiply(d,-1); // prefer a column operation here
    }

    // we prefer to start with column operations here, which need no recording
    for (size_t j=d+1; j<n; ++j)
      column_clear(M,d,j,d); // makes |M(d,d)==gcd>0| and |M(d,j)==0|
    // initial sweep is unlikely to be sufficient, so no termination test here

    size_t i=d+1,j;
    do // sweep column and row alternatively with |M(d,d)| until both cleared
    {
      for ( ; i<m; ++i)
	row_clear<C,false>(M,i,d,d,result);

      for (j=d+1; j<n; ++j)
	if (M(d,j)!=C(0))
	  break;

      if (j==n) // most likely because |row_clear| left column |d| unchanged
	break;

      for ( ; j<n; ++j)
	column_clear(M,d,j,d);

      for (size_t i=d+1; i<m; ++i)
	if (M(i,d)!=C(0))
	  break;
    }
    while(i<m); // then apparently some |column_clear| changed row |d|

    diagonal[d] = M(d,d);
  } // |for d|

  return result;
}


 // instantiations
typedef latticetypes::LatticeCoeff T;

template
void column_clear(matrix::Matrix<T>& M, size_t i, size_t j, size_t k);
template
void row_clear(matrix::Matrix<T>& M, size_t i, size_t j, size_t k);

template
bitmap::BitMap column_echelon<T>(matrix::Matrix<T>& M);

template
std::vector<T> diagonalise(matrix::Matrix<T> M, // by value
			   matrix::Matrix<T>& row,
			   matrix::Matrix<T>& col);

template
matrix::Matrix<T> Smith_basis(matrix::Matrix<T> M, // by value
			      std::vector<T>& diagonal);

template // an abomination due to |abelian::Endomorphism|
std::vector<unsigned long>
  diagonalise(matrix::Matrix<unsigned long> M, // by value
	      matrix::Matrix<unsigned long>& row,
	      matrix::Matrix<unsigned long>& col);

} // |namespace matreduc|
} // |namespace atlas|