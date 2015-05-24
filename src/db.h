/*************************************************************************
    > File Name: db.h
    > Author: Yibo Lin
    > Mail: yibolin@utexas.edu
    > Created Time: Thu 06 Nov 2014 09:04:57 AM CST
 ************************************************************************/

#ifndef _SIMPLEMPL_DB_H
#define _SIMPLEMPL_DB_H

#include <iostream>
#include <vector>
#include <map>
#include <set>
#include <boost/cstdint.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/array.hpp>
#include <boost/foreach.hpp>
#include <boost/typeof/typeof.hpp>
//#include <boost/polygon/polygon.hpp>
#include <boost/geometry.hpp>
// use adapted boost.polygon in boost.geometry, which is compatible to rtree
#include <boost/geometry/geometries/adapted/boost_polygon.hpp>
#include <boost/geometry/index/rtree.hpp>
#include <boost/tuple/tuple.hpp>

#if 0
// for rtree 
#include <boost/geometry.hpp>
#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/geometries/box.hpp>
#include <boost/geometry/index/rtree.hpp>
#endif 

#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>

// for Polygon2Rectangle
#include <limbo/geometry/api/BoostPolygonApi.h>
#include <limbo/geometry/Polygon2Rectangle.h>

namespace SimpleMPL {

using std::cout;
using std::endl;
using std::vector;
using std::map;
using std::set;

namespace gtl = boost::polygon;
namespace bg = boost::geometry;
namespace bgi = bg::index;
using boost::int32_t;
using boost::int64_t;
using boost::array;
using boost::shared_ptr;
using boost::make_shared;
using boost::tuple;
using gtl::point_concept;
using gtl::segment_concept;
using gtl::rectangle_concept;
using gtl::polygon_90_concept;
using gtl::polygon_90_set_concept;
using gtl::point_data;
using gtl::segment_data;
using gtl::rectangle_data;
using gtl::polygon_90_data;
using gtl::polygon_90_set_data;

using namespace gtl::operators;

template <typename T>
class Rectangle : public rectangle_data<T>
{
	public:
		typedef T coordinate_type;
		typedef rectangle_data<coordinate_type> base_type;
		typedef rectangle_concept geometry_type; // important 
		typedef point_data<coordinate_type> point_type;
		using typename base_type::interval_type;

		/// default constructor 
		Rectangle() : base_type()
		{
			this->initialize();
		}
		Rectangle(interval_type const& hor, interval_type const& ver) : base_type(hor, ver) 
		{
			this->initialize();
		}
		Rectangle(coordinate_type xl, coordinate_type yl, coordinate_type xh, coordinate_type yh) : base_type(xl, yl, xh, yh)
		{
			this->initialize();
		}
		/// copy constructor
		Rectangle(Rectangle const& rhs) : base_type(rhs)
		{
			this->copy(rhs);
		}
		/// assignment 
		Rectangle& operator=(Rectangle const& rhs)
		{
			if (this != &rhs)
			{
				this->base_type::operator=(rhs);
				this->copy(rhs);
			}
			return *this;
		}
		virtual ~Rectangle() {this->m_valid = false;}

		long internal_id() {return m_internal_id;}

		bool valid() const {return m_valid;}
		void valid(bool v) {m_valid = v;}

		int8_t color() const {return m_color;}
		void color(int8_t c) {m_color = c;}

		int32_t layer() const {return m_layer;}
		void layer(int32_t l) {m_layer = l;}

//		uint32_t comp_id() const {return m_comp_id;}
//		void comp_id(uint32_t c) {m_comp_id = c;}

		uint32_t pattern_id() const {return m_pattern_id;}
		void pattern_id(uint32_t p) {m_pattern_id = p;}

		// for debug 
		void check() 
		{
			assert_msg(this->valid(), "id = " << this->id() << " (" << gtl::xl(*this) << ", " << gtl::yl(*this) << ", " 
					<< gtl::xh(*this) << ", " << gtl::yh(*this) << ")");
			assert_msg(gtl::xl(*this) < gtl::xh(*this) && gtl::yl(*this) < gtl::yh(*this),
					"id = " << this->id() << " (" << gtl::xl(*this) << ", " << gtl::yl(*this) << ", " 
					<< gtl::xh(*this) << ", " << gtl::yh(*this) << ")");
		}

		friend std::ostream& operator<<(std::ostream& os, Rectangle const& rhs)
		{
			os << "(" << gtl::xl(rhs) << ", " << gtl::yl(rhs) << ", " << gtl::xh(rhs) << ", " << gtl::yh(rhs) << ")";
			return os;
		}

	private:
		void initialize()
		{
			m_color = m_layer = -1;
			//m_comp_id = std::numeric_limits<uint32_t>::max();
			m_pattern_id = std::numeric_limits<uint32_t>::max();
			m_valid = true;
#pragma omp critical 
			m_internal_id = generate_id();
		}
		void copy(Rectangle const& rhs)
		{
			this->m_color = rhs.m_color;
			this->m_layer = rhs.m_layer;
			//this->m_comp_id = rhs.m_comp_id;
			this->m_pattern_id = rhs.m_pattern_id;
			this->m_valid = rhs.m_valid;
			this->m_internal_id = rhs.m_internal_id;
		}
		static long generate_id()
		{
			static long cnt = -1;
			assert(cnt < std::numeric_limits<long>::max());
#pragma omp atomic 
			cnt += 1;
			return cnt;
		}

		long m_internal_id; ///< internal id 
		bool m_valid; ///< 1 valid, 0 invalid, default is true 

	protected:
		int8_t m_color; ///< color 
		int32_t m_layer; ///< input layer 
		//uint32_t m_comp_id; ///< independent component id 
		uint32_t m_pattern_id; ///< index in the pattern array 
};

template <typename T>
class Polygon : public polygon_90_data<T>
{
	public:
		typedef T coordinate_type;
		typedef polygon_90_data<coordinate_type> base_type;
		typedef Rectangle<coordinate_type> rectangle_type;
		typedef point_data<coordinate_type> point_type;
		typedef shared_ptr<rectangle_type> rectangle_pointer_type;
		using typename base_type::geometry_type;
		using typename base_type::compact_iterator_type;
		using typename base_type::iterator_type;
		using typename base_type::area_type;

		Polygon() : base_type() 
		{
			m_color = -1;
			this->initialize();
		}
		Polygon(Polygon const& rhs) : base_type(rhs)
		{
			m_color = rhs.m_color;
			this->initialize();
			//this->m_vPolyRect = rhs.m_vPolyRect;
			//this->m_bbox = rhs.m_bbox;
		}

#if 0
		//vector<rectangle_pointer_type> const& polyrect() const {return m_vPolyRect;}
		//vector<rectangle_pointer_type>& polyrect() {return m_vPolyRect;}
		rectangle_type bbox() const
		{
			coordinate_type xl = std::numeric_limits<coordinate_type>::max();
			coordinate_type yl = std::numeric_limits<coordinate_type>::max();
			coordinate_type xh = std::numeric_limits<coordinate_type>::min();
			coordinate_type yh = std::numeric_limits<coordinate_type>::min();
			for (BOOST_AUTO(it, this->begin()); it != this->end(); ++it)
			{
				xl = std::min(xl, gtl::x(*it));
				yl = std::min(yl, gtl::y(*it));
				xh = std::max(xh, gtl::x(*it));
				yh = std::max(yh, gtl::y(*it));
			}
			// gtl::construct<rectangle_type> is not defined
			// so here directly use rectangle_type's constructor
			return rectangle_type(xl, yl, xh, yh);
		}
#endif

	protected:
		void initialize()
		{
#pragma omp critical 
			m_id = generate_id();
		}
		static long generate_id()
		{
			static long cnt = -1;
			assert(cnt < std::numeric_limits<long>::max());
#pragma omp atomic 
			cnt += 1;
			return cnt;
		}

		long m_id; ///< internal id 
		//vector<rectangle_pointer_type> m_vPolyRect; ///< save decomposed rectangles 

		int32_t m_color; ///< color 
};

} // namespace SimpleMPL

/// API for Boost.Geometry 
namespace boost { namespace geometry { namespace index {

template <typename Box>
struct indexable< boost::shared_ptr<Box> >
{
    typedef boost::shared_ptr<Box> V;

    typedef Box const& result_type;
    result_type operator()(V const& v) const { return *v; }
};
}}} // namespace boost // namespace geometry // namespace index

namespace boost { namespace geometry { namespace traits {

//////// for rectangles ////////
template <typename CoordinateType>
struct tag<SimpleMPL::Rectangle<CoordinateType> > : public tag<typename SimpleMPL::Rectangle<CoordinateType>::base_type>
{};


template <typename CoordinateType>
struct point_type<SimpleMPL::Rectangle<CoordinateType> >
{
    typedef typename SimpleMPL::Rectangle<CoordinateType>::point_type type;
};


template <typename CoordinateType>
struct indexed_access
<
    SimpleMPL::Rectangle<CoordinateType>,
    min_corner, 0
> : public indexed_access<typename SimpleMPL::Rectangle<CoordinateType>::base_type, min_corner, 0>
{};


template <typename CoordinateType>
struct indexed_access
<
    SimpleMPL::Rectangle<CoordinateType>,
    min_corner, 1
> : public indexed_access<typename SimpleMPL::Rectangle<CoordinateType>::base_type, min_corner, 1>
{};


template <typename CoordinateType>
struct indexed_access
<
    SimpleMPL::Rectangle<CoordinateType>,
    max_corner, 0
> : public indexed_access<typename SimpleMPL::Rectangle<CoordinateType>::base_type, max_corner, 0>
{};


template <typename CoordinateType>
struct indexed_access
<
    SimpleMPL::Rectangle<CoordinateType>,
    max_corner, 1
> : public indexed_access<typename SimpleMPL::Rectangle<CoordinateType>::base_type, max_corner, 1>
{};

}}} // namespace boost // namespace geometry // namespace traits

namespace SimpleMPL {

/// current implementation assume all the input patterns are rectangles 
template <typename T>
struct LayoutDB : public rectangle_data<T>
{
	typedef T coordinate_type;
	typedef typename gtl::coordinate_traits<coordinate_type>::manhattan_area_type area_type;
	typedef typename gtl::coordinate_traits<coordinate_type>::coordinate_difference coordinate_difference;
	typedef rectangle_data<coordinate_type> base_type;
	typedef point_data<coordinate_type> point_type;
	typedef Rectangle<coordinate_type> rectangle_type;
	typedef Polygon<coordinate_type> polygon_type;
	typedef segment_data<coordinate_type> path_type;
	typedef shared_ptr<polygon_type> polygon_pointer_type;
	typedef shared_ptr<rectangle_type> rectangle_pointer_type;
	typedef bgi::rtree<rectangle_pointer_type, bgi::linear<16, 4> > rtree_type;
	typedef polygon_90_set_data<coordinate_type> polygon_set_type;

	rtree_type tPattern; ///< rtree for components that intersects the LayoutDB
	vector<rectangle_pointer_type> vPattern; ///< uncolored and precolored patterns 
	map<int32_t, vector<path_type> > hPath; ///< path 

	/// options 
	set<int32_t> sUncolorLayer; ///< layers that represent uncolored patterns 
	set<int32_t> sPrecolorLayer; ///< layers that represent precolored features, they should have the same number of colors 
	set<int32_t> sPathLayer; ///< path layers that represent conflict edges 
	coordinate_difference coloring_distance; ///< minimum coloring distance 
	int32_t color_num; ///< number of colors available, only support 3 or 4
	uint32_t thread_num; ///< number of maximum threads for parallel computation 

	string input_gds; ///< input gdsii filename 
	string output_gds; ///< output gdsii filename 

	LayoutDB() : base_type() 
	{
		initialize();
	}
	LayoutDB(coordinate_type xl, coordinate_type yl, coordinate_type xh, coordinate_type yh) : base_type(xl, yl, xh, yh) 
	{
		initialize();
	}
	LayoutDB(LayoutDB const& rhs) : base_type(rhs)
	{
		copy(rhs);
	}
	~LayoutDB()
	{
	}

	LayoutDB& operator=(LayoutDB const& rhs)
	{
		if (this != &rhs)
		{
			this->base_type::operator=(rhs);
			copy(rhs);
		}
		return *this;
	}
	void initialize()
	{
		coloring_distance = 0;
		color_num = 3;
		thread_num = 1;
		input_gds = "";
		output_gds = "";
	}
	void copy(LayoutDB const& rhs)
	{
		tPattern = rhs.tPattern;
		vPattern = rhs.vPattern;
		hPath = rhs.hPath;
		// options
		sUncolorLayer = rhs.sUncolorLayer;
		sPrecolorLayer = rhs.sPrecolorLayer;
		sPathLayer = rhs.sPathLayer;
		coloring_distance = rhs.coloring_distance;
		color_num = rhs.color_num;
		thread_num = rhs.thread_num;
		input_gds = rhs.input_gds;
		output_gds = rhs.output_gds;
	}

	void add_pattern(int32_t layer, vector<point_type> const& vPoint)
	{
		assert(vPoint.size() >= 4 && vPoint.size() < 6);

		rectangle_pointer_type pPattern(new rectangle_type());
		for (typename vector<point_type>::const_iterator it = vPoint.begin(); it != vPoint.end(); ++it)
		{
			if (it == vPoint.begin())
				gtl::set_points(*pPattern, *it, *it);
			else 
				gtl::encompass(*pPattern, *it);
		}
		pPattern->layer(layer);
		pPattern->pattern_id(vPattern.size());

		// update layout boundary 
		if (vPattern.empty()) // first call 
			gtl::assign(*this, *pPattern);
		else // bloat layout region to encompass current points 
			gtl::encompass(*this, *pPattern);

		// collect patterns 
		bool pattern_layer_flag = true;
		if (sPrecolorLayer.count(layer)) 
		{
			// for precolored patterns 
			// set colors 
			pPattern->color(layer-*sPrecolorLayer.begin());
		}
		else if (sUncolorLayer.count(layer))
		{
			// for uncolored patterns
		}
		else pattern_layer_flag = false;

		if (pattern_layer_flag)
		{
#ifdef DEBUG
//			if (gtl::xl(*pPattern) == 63194 && gtl::yl(*pPattern) == 41616)
//				printf("%d, %d, %d, %d, %d\n", pPattern->layer(), gtl::xl(*pPattern), gtl::yl(*pPattern), gtl::xh(*pPattern), gtl::yh(*pPattern));
#endif
			// check duplicate and overlaps 
			// avoid all overlaps 
			vector<rectangle_pointer_type> vDupPattern; 
			tPattern.query(bgi::intersects(*pPattern), std::back_inserter(vDupPattern));
			if (!vDupPattern.empty()) 
			{
				cout << "Warning: " << *pPattern << " overlaps with ";
				for (typename vector<rectangle_pointer_type>::const_iterator it = vDupPattern.begin(); it != vDupPattern.end(); ++it)
					cout << **it << " ";
				cout << "ignored"<< endl;
			}
			else 
			{
				// collect pattern 
				vPattern.push_back(pPattern);
				// add to rtree 
				tPattern.insert(pPattern);
			}
		}
	}
	void add_path(int32_t layer, vector<point_type> const& vPoint)
	{
		if (vPoint.size() < 2) return;
		for (typename vector<point_type>::const_iterator it = vPoint.begin()+1; it != vPoint.end(); ++it)
		{
			path_type p (*(it-1), *it);
			if (hPath.count(layer))
				hPath[layer].push_back(p);
			else assert(hPath.insert(make_pair(layer, vector<path_type>(1, p))).second);
		}
	}
};

} // namespace SimpleMPL

/// API for Boost.Polygon
namespace boost { namespace polygon {

/// necessary for customized rectangle types 
template <typename T>
struct geometry_concept<SimpleMPL::Rectangle<T> > 
{
	typedef rectangle_concept type;
};
template <typename T>
struct geometry_concept<SimpleMPL::LayoutDB<T> > 
{
	typedef rectangle_concept type;
};

/// bug in boost library in the following function
/// function intersects and intersect do not always return the same results (when consider_touch = false)
/// create a specialization to resolve it 
  template <typename T>
  typename enable_if< typename gtl_and_3<y_r_b_intersect3, typename is_mutable_rectangle_concept<typename geometry_concept<SimpleMPL::Rectangle<T> >::type>::type,
                                         typename is_rectangle_concept<typename geometry_concept<SimpleMPL::Rectangle<T> >::type>::type>::type,
                       bool>::type
  intersect(SimpleMPL::Rectangle<T>& rectangle, const SimpleMPL::Rectangle<T>& b, bool consider_touch = true) {
	  // the original version is "intersects(rectangle, b)" without consider_touch 
    if(intersects(rectangle, b, consider_touch)) {
      intersect(rectangle, horizontal(b), HORIZONTAL, consider_touch);
      intersect(rectangle, vertical(b), VERTICAL, consider_touch);
      return true;
    }
    return false;
  }

}} // namespace boost // namespace polygon

/// API for limbo::geometry
namespace limbo { namespace geometry {

/// \brief specialization for SimpleMPL::Rectangle
template <typename T>
struct rectangle_traits<SimpleMPL::Rectangle<T> >// : public rectangle_traits<boost::polygon::rectangle_data<T> >
{
	typedef T coordinate_type;
	typedef SimpleMPL::Rectangle<coordinate_type> rectangle_type;

	static coordinate_type get(const typename rectangle_type::base_type& rect, direction_2d const& dir) 
	{
		switch (dir)
		{
			case LEFT: return boost::polygon::xl(rect);
			case BOTTOM: return boost::polygon::yl(rect);
			case RIGHT: return boost::polygon::xh(rect);
			case TOP: return boost::polygon::yh(rect);
			default: assert_msg(0, "unknown direction_2d type");
		}
	}
	static void set(typename rectangle_type::base_type& rect, direction_2d const& dir, coordinate_type const& value) 
	{
		switch (dir)
		{
			case LEFT: boost::polygon::xl(rect, value); break;
			case BOTTOM: boost::polygon::yl(rect, value); break;
			case RIGHT: boost::polygon::xh(rect, value); break;
			case TOP: boost::polygon::yh(rect, value); break;
			default: assert_msg(0, "unknown direction_2d type");
		}
	}
	static rectangle_type construct(coordinate_type const& xl, coordinate_type const& yl, 
			coordinate_type const& xh, coordinate_type const& yh) 
	{
		return rectangle_type(xl, yl, xh, yh); 
	}
};

}} // namespace limbo // namespace geometry


#endif 