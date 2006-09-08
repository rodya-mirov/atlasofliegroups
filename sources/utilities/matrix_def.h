/*!
\file
  This is matrix_def.h. This module contains some simple utilities for
  matrices. When we will need to do stuff for large matrices, we will
  need to look elsewhere, or in any case think a lot more.
*/
/*
  Copyright (C) 2004,2005 Fokko du Cloux
  part of the Atlas of Reductive Lie Groups

  See file main.cpp for full copyright notice
*/

#include "intutils.h"

/*****************************************************************************

  This module contains some simple utilities for matrices. When we will need
  to do stuff for large matrices, we will need to look elsewhere, or in any
  case think a lot more.

******************************************************************************/

namespace atlas {

namespace matrix {

namespace {

template<typename C> void blockReduce(Matrix<C>&, size_t, Matrix<C>&,
				      Matrix<C>&);
template<typename C> void blockShape(Matrix<C>&, size_t, Matrix<C>&,
				     Matrix<C>&);
template<typename C> void columnReduce(Matrix<C>&, size_t, size_t, Matrix<C>&);
template<typename C> bool hasBlockReduction(const Matrix<C>&, size_t);
template<typename C> bool hasReduction(const Matrix<C>&, size_t);
template<typename C> typename Matrix<C>::index_pair
  findBlockReduction(const Matrix<C>&, size_t);
template<typename C> typename Matrix<C>::index_pair
  findReduction(const Matrix<C>&, size_t);
template<typename C> void rowReduce(Matrix<C>&, size_t, size_t, Matrix<C>&);

}

}

/*****************************************************************************

        Chapter I -- The Matrix class

  We implement a matrix simply as a vector of elements, concatenating the
  rows.

******************************************************************************/

namespace matrix {

/******** constructors *******************************************************/

template<typename C>
Matrix<C>::Matrix(const std::vector<std::vector<C> >& b)

/*!
  This constructor constructs a matrix from a bunch of vectors, columnwise.
  It is assumed that all elements of b have the same size.
*/

{
  if (b.empty()) { // empty matrix
    d_rows = 0;
    d_columns = 0;
    return;
  }

  d_rows = b[0].size();
  d_columns = b.size();
  d_data.resize(d_rows*d_columns);

  for (size_t j = 0; j < d_columns; ++j) {
    for (size_t i = 0; i < d_rows; ++i)
      (*this)(i,j) = b[j][i];
  }
}

template<typename C>
Matrix<C>::Matrix(const Matrix<C>& m, const std::vector<std::vector<C> >& b)
  :d_data(m.d_data), d_rows(m.d_rows), d_columns(m.d_columns)

/*!
  This constructor constructs a square matrix, which is the matrix
  representing the operator defined by m in the canonical basis, in
  the basis b.
*/

{
  Matrix<C> p(b);
  invConjugate(*this,p);
}

template<typename C>
Matrix<C>::Matrix(const Matrix<C>& source, size_t r_first, size_t c_first,
		  size_t r_last, size_t c_last)
  :d_data((r_last-r_first)*(c_last-c_first)),
   d_rows(r_last-r_first),
   d_columns(c_last-c_first)

/*!
  Synopsis: constructs the matrix corresponding to the block [r_first,r_last[
  x [c_first,c_last[ of source.
*/

{
  for (size_t j = 0; j < d_columns; ++j)
    for (size_t i = 0; i < d_rows; ++i)
      (*this)(i,j) = source(r_first+i,c_first+j);
}

template<typename C> template<typename I>
Matrix<C>::Matrix(const Matrix<C>& m, const I& first, const I& last)
  :d_data(m.d_data), d_rows(m.d_rows), d_columns(m.d_columns)

/*!
  Here I is an iterator whose value type is Weight.

  This constructor constructs a square matrix, which is the matrix
  representing the operator defined by m in the canonical basis, in
  the basis supposed to be contained in [first,last[.
*/

{
  Matrix<C> p(first,last,tags::IteratorTag());
  invConjugate(*this,p);
}

template<typename C>
template<typename I> Matrix<C>::Matrix(const I& first, const I& last,
				       tags::IteratorTag)

/*!
  Here I is an iterator type whose value type should be vector<C>.
  The idea is that we read the columns of the matrix from the iterator.
  However, in order to be able to determine the allocation size,
  and since unfortunately we decided to read the matrix in rows, we
  need to catch the vectors first.

  Of course it is assumed that all the vectors in the range have the
  same size.
*/

{
  std::vector<std::vector<C> > b(first,last);

  if (b.empty()) {
    d_rows = 0;
    d_columns = 0;
    return;
  }

  d_rows = b[0].size();
  d_columns = b.size();
  d_data.resize(d_rows*d_columns);

  for (size_t i = 0; i < d_rows; ++i)
    for (size_t j = 0; j < d_columns; ++j)
      (*this)(i,j) = b[j][i];
}

/******** accessors **********************************************************/

template<typename C>
bool Matrix<C>::operator== (const Matrix<C>& m) const

{
  if ((d_rows != m.d_rows) or (d_columns != m.d_columns))
    return false;

  return d_data == m.d_data;
}

template<typename C>
typename Matrix<C>::index_pair Matrix<C>::absMinPos(size_t i_min,
						    size_t j_min) const


/*!
  Returns the position of the smallest non-zero entry in absolute value,
  in the region i >= i_min, j >= j_min
*/

{
  C minCoeff = std::numeric_limits<C>::max();
  size_t i_m = d_rows;
  size_t j_m = d_columns;

  for (size_t i = i_min; i < d_rows; ++i)
    for (size_t j = j_min; j < d_columns; ++j) {
      C c = (*this)(i,j);
      if (c == 0)
	continue;
    if (intutils::abs(c) < minCoeff) { // new smallest value
      minCoeff = intutils::abs(c);
      i_m = i;
      j_m = j;
      if (minCoeff == 1)
	break;
    }
    }

  return std::make_pair(i_m,j_m);
}

template<typename C>
void Matrix<C>::apply(std::vector<C>& v, const std::vector<C>& w) const

/*!
  Applies the matrix to the vector w, and puts the result in v. It is
  assumed that the size of w is the number of columns, and that the size
  of v is the number of rows.
*/

{
  std::vector<C> tmp = w; // safeguard in case v = w

  for (size_t i = 0; i < d_rows; ++i) {
    C c = 0;
    for (size_t j = 0; j < d_columns; ++j) {
      C m_ij = (*this)(i,j);
      c += m_ij*tmp[j];
    }
    v[i] = c;
  }

  return;
}

template<typename C> template<typename I, typename O>
void Matrix<C>::apply(const I& first, const I& last, O out) const

/*!
  A pipe-version of apply. We assume that I is an InputIterator with
  value-type vector<C>, and O an OutputIterator with the same
  value-type. Then we apply our matrix to each vector in [first,last[
  and output it to out.
*/

{
  for (I i = first; i != last; ++i) {
    std::vector<C> v(d_rows);
    apply(v,*i);
    *out++ = v;
  }

  return;
}

template<typename C>
void Matrix<C>::column(std::vector<C>& v, size_t j) const

/*!
  Puts the j-th column of the matrix in v.
*/

{
  v.resize(d_rows);

  for (size_t i = 0; i < d_rows; ++i)
    v[i] = (*this)(i,j);

  return;
}

template<typename C>
bool Matrix<C>::divisible(C c) const

/*!
  Tells if all coefficients of the matrix are divisible by c.
*/

{
  for (size_t j = 0; j < d_data.size(); ++j)
    if (d_data[j]%c)
      return false;

  return true;
}

template<typename C>
void Matrix<C>::row(std::vector<C>& v, size_t i) const

/*!
  Puts the i-th row of the matrix in v.
*/

{
  v.resize(d_columns);

  for (size_t j = 0; j < d_columns; ++j)
    v[j] = (*this)(i,j);

  return;
}

/******** manipulators *******************************************************/

template<typename C>
Matrix<C>& Matrix<C>::operator+= (const Matrix<C>&  m)

/*!
  Incrementation by addition with m. It is assumed that m and *this
  have the same shape.
*/

{
  for (size_t k = 0; k < d_data.size(); ++k) {
    d_data[k] += m.d_data[k];
  }
}

template<typename C>
Matrix<C>& Matrix<C>::operator-= (const Matrix<C>&  m)

/*!
  Incrementation by subtraction of m. It is assumed that m and *this
  have the same shape.
*/

{
  for (size_t k = 0; k < d_data.size(); ++k) {
    d_data[k] -= m.d_data[k];
  }
}

template<typename C>
Matrix<C>& Matrix<C>::operator*= (const Matrix<C>&  m)

/*!
  Incrementation by right multiplication with m. It is assumed that the
  number of rows of m is equal to the number of columns of *this.
*/

{
  C zero = 0; // conversion from int
  Matrix<C> prod(d_rows,m.d_columns,zero);

  for (size_t i = 0; i < d_rows; ++i)
    for (size_t j = 0; j < m.d_columns; ++j) {
      for (size_t k = 0; k < d_columns; ++k) {
	C t_ik = (*this)(i,k);
	C m_kj = m(k,j);
	prod(i,j) += t_ik * m_kj;
      }
    }

  swap(prod);

  return *this;
}

template<typename C>
Matrix<C>& Matrix<C>::operator/= (const C& c)

/*!
  Divides all entries of the matrix by c.
*/

{
  if (c == 1)
    return *this;

  iterator e = end();

  for (iterator i = begin(); i != e; ++i)
    *i /= c;

  return *this;
}

template<typename C>
void Matrix<C>::changeColumnSign(size_t j)

/*!
  Changes the sign of all the entries in column j.
*/

{
  for (size_t i = 0; i < d_rows; ++i) {
    (*this)(i,j) *= -1;
  }

  return;
}

template<typename C>
void Matrix<C>::changeRowSign(size_t i)

/*!
  Changes the sign of all the entries in row i.
*/

{
  for (size_t j = 0; j < d_columns; ++j) {
    (*this)(i,j) *= -1;
  }

  return;
}

template<typename C>
void Matrix<C>::columnOperation(size_t i, size_t j, const C& c)

/*!
  Carries out the column operation consisting of adding c times column j
  to column i.
*/

{
  for (size_t k = 0; k < columnSize(); ++k)
    (*this)(k,i) += c*(*this)(k,j);

  return;
}

template<typename C>
void Matrix<C>::copy(const Matrix<C>& source, size_t r, size_t c)

/*!
  Copies source to the rectangle of the current matrix with upper left corner
  at (r,c) and the appropriate size.
*/

{
  for (size_t j = 0; j < source.d_columns; ++j)
    for (size_t i = 0; i < source.d_rows; ++i)
      (*this)(r+i,c+j) = source(i,j);

  return;
}

template<typename C>
void Matrix<C>::copyColumn(const Matrix<C>& m, size_t c_d, size_t c_s)

/*!
  Copies the column c_s of the matrix m to the column c_d of the current
  matrix. It is assumed that the two columns have the same size.
*/

{
  for (size_t i = 0; i < d_rows; ++i)
    (*this)(i,c_d) = m(i,c_s);

  return;
}

template<typename C>
void Matrix<C>::copyRow(const Matrix<C>& m, size_t r_d, size_t r_s)

/*!
  Copies the column r_s of the matrix m to the column r_d of the current
  matrix. It is assumed that the two columns have the same size.
*/

{
  for (size_t j = 0; j < d_columns; ++j)
    (*this)(r_d,j) = m(r_s,j);

  return;
}

template<typename C>
void Matrix<C>::eraseColumn(size_t j)

/*!
  Erases column j from the matrix.
*/

{
  iterator pos = begin() + j;
  --d_columns;

  for (size_t k = 0; k < d_rows; ++k, pos += d_columns)
    d_data.erase(pos);

  return;
}

template<typename C>
void Matrix<C>::eraseRow(size_t i)

/*!
  Erases row i from the matrix.
*/

{
  iterator first = begin() + i*d_columns;
  d_data.erase(first,first+d_columns);
  --d_rows;

  return;
}

template<typename C> void Matrix<C>::invert()

/*!
  Here we invert the matrix without catching the denominator. The intent
  is that it should be used for invertible matrices only.
*/

{
  C d;
  invert(d);

  return;
}

template<typename C>
void Matrix<C>::invert(C& d)

/*!
  This function inverts the matrix M. It is assumed that the coefficents
  are in an integral domain. At the conclusion of the process, d will contain
  the denominator for the inverted matrix (so that the true result is M/d).

  If the matrix is not invertible, the resulting matrix is undefined, and
  d is set to 0.

  We have chosen here to avoid divisions as much as possible. So in fact we
  take the matrix to Smith normal form through elementary operations; from
  there we deduce the smallest possible denominator for the inverse, and
  the inverse matrix. The difference with SmithNormal is that we have to
  keep track of both row and column operations.

  NOTE : probably the Smith normal stuff can be streamlined a bit so that
  functions from there can be called, instead of rewriting things slightly
  differently as we do here. In particular, the accounting of the row
  operations seems a bit different here from there.
*/

{
  if (d_rows == 0) // do nothing
    return;

  C zero = 0; // conversion from int should work!
  Matrix<C> row(d_rows,d_rows,zero);

  // set res to identity

  for (size_t j = 0; j < d_rows; ++j)
    row(j,j) = 1;

  Matrix<C> col(row);

  // take *this to triangular form

  for (size_t r = 0; r < d_rows; ++r) {

    if (isZero(r,r)) { // null matrix left; matrix is not invertible
      d = 0;
      return;
    }

    index_pair k = absMinPos(r,r);

    if (k.first > r) {
      swapRows(r,k.first);
      row.swapRows(r,k.first);
    }

    if(k.second > r) {
      swapColumns(r,k.second);
      col.swapColumns(r,k.second);
    }

    if ((*this)(r,r) < 0) {
      changeRowSign(r);
      row.changeRowSign(r);
    }

    // ensure m(0,0) divides row and column

    while (hasReduction(*this,r)) {
      k = findReduction(*this,r);
      if (k.first > r) { // row reduction
	rowReduce(*this,k.first,r,row);
      }
      else { // column reduction
	columnReduce(*this,k.second,r,col);
      }
    }

    // clean up row and column

    blockShape(*this,r,row,col);
    blockReduce(*this,r,row,col);

  } // next |r|

  // now multiply out diagonal

  for (size_t j = 1; j < d_rows; ++j) {
    (*this)(j,j) *= (*this)(j-1,j-1);
  }

  // and write result to |d| and to |*this|

  d = (*this)(d_rows-1,d_rows-1); // minimal denominator

  for (size_t j = 0; j < d_rows; ++j) {
    (*this)(j,j) = d/(*this)(j,j);
  }

  col *= *this;
  col *= row;
  swap(col);

  return;
}

template<typename C>
bool Matrix<C>::isZero(size_t i_min, size_t j_min) const

/*!
  Whether all the entries in rectangle starting at |(i_min,i_max)| are zero.
*/

{
  if (d_data.size() == 0) // empty matrix
    return true;

  for (size_t i = i_min; i < d_rows; ++i)
    for (size_t j = j_min; j < d_columns; ++j)
      if ((*this)(i,j))
	return false;

  return true;
}

template<typename C>
void Matrix<C>::negate()

/*!
  Synopsis: change the sign of the matrix.
*/

{
  for (size_t i = 0; i < d_rows; ++i)
    for (size_t j = 0; j < d_rows; ++j)
      (*this)(i,j) = -(*this)(i,j);

  return;
}

template<typename C>
void Matrix<C>::permute(const setutils::Permutation& a)

/*!
  Synopsis : permutes the matrix according to the basis permutation a.

  Precondition : m is a square matrix of order n; a holds a permutation
  of the matrix n;

  Postcondition : m holds the matrix of the same operator in the basis
  e_{a[0]}, ... , e_{a[n-1]}.

  NOTE : we do it the lazy way, using a copy.
*/

{
  Matrix<C> q(d_columns);

  for (size_t j = 0; j < d_columns; ++j)
    for (size_t i = 0; i < d_rows; ++i) {
      size_t a_i = a[i];
      size_t a_j = a[j];
      q(i,j) = (*this)(a_i,a_j);
    }

  swap(q);

  return;
}

template<typename C>
void Matrix<C>::resize(size_t m, size_t n)

/*!
  Synopsis: changes the size of the matrix to m x n.

  NOTE: this is a bad name, and a bit dangerous. One would expect the data
  of the matrix in the upper left corner to be preserved, but this is not
  done; in other words, the contents of the resized matrix are garbage.
*/

{
  d_data.resize(m*n);
  d_rows = m;
  d_columns = n;

  return;
}

template<typename C>
void Matrix<C>::resize(size_t m, size_t n, const C& c)

/*!
  Synopsis: changes the size of the matrix to m x n, and resets _all_ elements
  to c.

  NOTE: this is a bad name, because it does not behave like resize for vectors.
*/

{
  d_data.assign(m*n,c);
  d_rows = m;
  d_columns = n;

  return;
}

template<typename C>
void Matrix<C>::rowOperation(size_t i, size_t j, const C& c)

/*!
  Carries out the row operation consisting of adding c times row j
  to row i.
*/

{
  for (size_t k = 0; k < rowSize(); ++k)
    (*this)(i,k) += c*(*this)(j,k);

  return;
}

template<typename C>
void Matrix<C>::swap(Matrix<C>& m)

/*!
  Swaps m with the current matrix.
*/

{
  // swap data

  d_data.swap(m.d_data);

  // swap row and column numbers

  std::swap(d_rows,m.d_rows);
  std::swap(d_columns,m.d_columns);

  return;
}

template<typename C>
void Matrix<C>::swapColumns(size_t i, size_t j)

/*!
  Interchanges columns i and j
*/

{
  for (size_t k = 0; k < columnSize(); ++k) {
    C tmp = (*this)(k,i);
    (*this)(k,i) = (*this)(k,j);
    (*this)(k,j) = tmp;
  }

  return;
}

template<typename C>
void Matrix<C>::swapRows(size_t i, size_t j)

/*!
  Interchanges rows i and j
*/

{
  for (size_t k = 0; k < rowSize(); ++k) {
    C tmp = (*this)(i,k);
    (*this)(i,k) = (*this)(j,k);
    (*this)(j,k) = tmp;
  }

  return;
}

template<typename C> void Matrix<C>::transpose()

/*!
  Transposes the matrix. We allow ourselves a temporary vector if the matrix
  is not square; it could likely be done in-place, but the permutation is
  rather complicated!
*/

{
  if (d_rows == d_columns) {// matrix is square
    for (size_t j = 0; j < d_columns; ++j)
      for (size_t i = j+1; i < d_rows; ++i) {
	C t = (*this)(i,j);
	(*this)(i,j) = (*this)(j,i);
	(*this)(j,i) = t;
      }
    return;
  }

  // now assume matrix is not square

  C zero = 0; // conversion from int
  std::vector<C> v(d_data.size(),zero);
  size_t k = 0;

  // write data for transpose matrix in v

  for (size_t j = 0; j < d_columns; ++j)
    for (size_t i = 0; i < d_rows; ++i) {
      v[k] = (*this)(i,j);
      k++;
    }

  // swap data and row- and column- size

  d_data.swap(v);

  size_t t = d_rows;
  d_rows = d_columns;
  d_columns = t;

  return;
}

}

/*****************************************************************************

        Chapter II --- Functions declared in matrix.h

******************************************************************************/

namespace matrix {

template<typename C>
  void columnVectors(std::vector<std::vector<C> >& b, const Matrix<C>& m)

/*!
  Synopsis: writes in b the list of column vectors of m.
*/

{
  b.resize(m.numColumns());

  for (size_t j = 0; j < b.size(); ++j) {
    m.column(b[j],j);
  }

  return;
}

template<typename C>
Matrix<C>& conjugate(Matrix<C>& m, const Matrix<C>& p)

/*!
  Conjugates m by p, i.e. transforms m into pmp^{-1}. It is assumed that
  p is invertible (over the quotient field of the coefficients), and that
  denominators cancel out.
*/

{
  Matrix<C> tmp(p);
  C d;
  tmp.invert(d);
  leftProd(tmp,m);
  leftProd(tmp,p);
  tmp /= d;
  m.swap(tmp);

  return m;
}

template<typename C>
void extractBlock(Matrix<C>& dest, const Matrix<C>& source, size_t firstRow,
		  size_t lastRow, size_t firstColumn, size_t lastColumn)

/*!
  Synopsis: sets dest equal to the block [firstRow,lastRow[ x [firstColumn,
  lastColumn[ in source.
*/

{
  dest.resize(lastRow - firstRow, lastColumn - firstColumn);

  for (size_t j = 0; j < dest.numColumns(); ++j)
    for (size_t i = 0; i < dest.numRows(); ++i)
      dest(i,j) = source(firstRow+i,firstColumn+j);

  return;
}

template<typename C>
void extractMatrix(Matrix<C>& dest, const Matrix<C>& source,
		   const std::vector<size_t>& r, const std::vector<size_t>& c)

/*!
  Synopsis: extracts the sub-matrix corresponding to the subsets r and c of the
  row and column indices respectively.
*/

{
  dest.resize(r.size(),c.size());

  for (size_t j = 0; j < c.size(); ++j)
    for (size_t i = 0; i < c.size(); ++i)
      dest(i,j) = source(r[i],c[j]);

  return;
}

template<typename C> void identityMatrix(Matrix<C>& m, size_t n)

/*!
  Synopsis: puts in q the identity matrix of size n.
*/

{
  C zero = 0; // conversion from int
  m.resize(n,n,zero);

  for (size_t j = 0; j < n; ++j)
    m(j,j) = 1;

  return;
}

template<typename C> void initBasis(std::vector<std::vector<C> >& b, size_t r)

/*!
  Synopsis: sets b to the canonical basis in dimension r.
*/

{
  C zero = 0; // conversion from int
  b.assign(r,std::vector<C>(r,zero));

  for (size_t j = 0; j < r; ++j) {
    b[j][j] = 1;
  }

  return;
}

template<typename C> Matrix<C>& invConjugate(Matrix<C>& m, const Matrix<C>& p)

/*!
  Conjugates m by p^{-1}, i.e. transforms m into p^{-1}mp. It is assumed that
  p is invertible (over the quotient field of the coefficients), and that
  denominators cancel out.
*/

{
  Matrix<C> tmp(p);
  C d;
  tmp.invert(d);
  tmp *= m;
  tmp *= p;
  tmp /= d;
  m.swap(tmp);

  return m;
}

template<typename C> Matrix<C>& leftProd(Matrix<C>& m, const Matrix<C>& p)

/*!
  Like operator *=, but for left multiplication of m by p.
*/

{
  Matrix<C> tmp(p);
  tmp *= m;
  m.swap(tmp);

  return m;
}

}

/*****************************************************************************

        Chapter III --- Auxiliary functions

******************************************************************************/

namespace matrix {

namespace {

template<typename C>
void blockReduce(Matrix<C>& m, size_t d, Matrix<C>& r, Matrix<C>& c)

/*!
  Ensures that m(d,d) divides the block from (d+1,d+1) down.
*/

{
  if (m(d,d) == 1) // no reduction
    return;

  while(hasBlockReduction(m,d)) {
    typename Matrix<C>::index_pair k = findBlockReduction(m,d);
    C one = 1; // conversion from int
    m.rowOperation(d,k.first,one);
    r.rowOperation(d,k.first,one);
    while (hasReduction(m,d)) {
      k = findReduction(m,d);
      if (k.first > d) { // row reduction
	rowReduce(m,k.first,d,r);
      }
      else { // column reduction
	columnReduce(m,k.second,d,c);
      }
    }
    blockShape(m,d,r,c);
  }

  if (m(d,d) > 1) // divide
    for (size_t j = d+1; j < m.rowSize(); ++j)
      for (size_t i = d+1; i < m.columnSize(); ++i)
	m(i,j) /= m(d,d);

  return;
}

template<typename C>
void blockShape(Matrix<C>& m, size_t d, Matrix<C>& r,
		Matrix<C>& c)

/*!
  Does the final reduction of m to block shape, recording row reductions in
  r and column reductions in c.
*/

{
  C a = m(d,d);

  for (size_t j = d+1; j < m.rowSize(); ++j) {
    if (m(d,j) == 0)
      continue;
    C q = m(d,j)/a;
    q = -q;
    m.columnOperation(j,d,q);
    c.columnOperation(j,d,q);
  }

  for (size_t i = d+1; i < m.columnSize(); ++i) {
    if (m(i,d) == 0)
      continue;
    C q = m(i,d)/a;
    q = -q;
    m.rowOperation(i,d,q);
    r.rowOperation(i,d,q);
  }

  return;
}

template<typename C>
void columnReduce(Matrix<C>& m, size_t j,
		  size_t d, Matrix<C>& c)

/*!
  Does the column reduction for m at place j, and does the same operation
  on r. The reduction consists in subtracting from column j the multiple
  of column d which leaves at (d,j) the remainder of the Euclidian division
  of m(d,j) by m(d,d), and then swapping columns j and d.
*/

{
  C a = m(d,d);
  C q = intutils::divide(m(d,j),a);
  q = -q;
  m.columnOperation(j,d,q);
  c.columnOperation(j,d,q);
  m.swapColumns(d,j);
  c.swapColumns(d,j);

  return;
}

template<typename C>
typename Matrix<C>::index_pair findBlockReduction(const Matrix<C>& m,
					     size_t r)

/*!
  Returns the reduction point of m. Assumes that hasBlockReduction(m,r) has
  returned true.
*/

{
  C a = m(r,r);

  for (size_t j = r+1; j < m.rowSize(); ++j) {
    for (size_t i = r+1; i < m.columnSize(); ++i) {
      if (m(i,j)%a)
	return std::make_pair(i,j);
    }
  }

  // this should never be reached
  return std::make_pair(static_cast<size_t>(0ul),static_cast<size_t>(0ul));
}

template<typename C>
typename Matrix<C>::index_pair findReduction(const Matrix<C>& m,
					     size_t r)

/*!
  Returns the reduction point of m. Assumes that hasReduction(m,r) has
  returned true.
*/

{
  C a = m(r,r);

  for (size_t j = r+1; j < m.rowSize(); ++j) {
    if (m(r,j)%a)
      return std::make_pair(r,j);
  }

  for (size_t i = r+1; i < m.columnSize(); ++i) {
    if (m(i,r)%a)
      return std::make_pair(i,r);
  }

  // this should never be reached
  return std::make_pair(static_cast<size_t>(0ul),static_cast<size_t>(0ul));
}

template<typename C>
bool hasBlockReduction(const Matrix<C>& m, size_t r)

/*!
  Tells if there is an element in the block under (r,r) which is not divisible
  bu m(r,r)
*/

{
  C a = m(r,r);

  if (a == 1)
    return false;

  for (size_t j = r+1; j < m.rowSize(); ++j) {
    for (size_t i = r+1; i < m.columnSize(); ++i) {
      if (m(i,j)%a)
	return true;
    }
  }

  return false;
}

template<typename C>
bool hasReduction(const Matrix<C>& m, size_t r)

/*!
  Tells if there is an element in the first row or column not divisible by
  m(r,r).
*/

{
  C a = m(r,r);

  if (a == 1)
    return false;

  for (size_t j = r+1; j < m.rowSize(); ++j) {
    if (m(r,j)%a)
      return true;
  }

  for (size_t i = r+1; i < m.columnSize(); ++i) {
    if (m(i,r)%a)
      return true;
  }

  return false;
}

template<typename C>
void rowReduce(Matrix<C>& m, size_t i, size_t d, Matrix<C>& r)

/*!
  Does the row reduction for m at place j, and does the same operation
  on r. The reduction consists in subtracting from row i the multiple
  of row d which leaves at (i,d) the remainder of the Euclidian division
  of m(d,j) by m(d,d), and then swapping rows i and d.
*/

{
  C a = m(d,d);
  C q = intutils::divide(m(i,d),a);
  q = -q;
  m.rowOperation(i,d,q);
  r.rowOperation(i,d,q);
  m.swapRows(d,i);
  r.swapRows(d,i);

  return;
}

}

}

}
