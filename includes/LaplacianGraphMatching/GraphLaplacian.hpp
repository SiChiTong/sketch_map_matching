#ifndef SKETCHALGORITHMS_GRAPHLAPLACIAN_251108
#define SKETCHALGORITHMS_GRAPHLAPLACIAN_251108

#include "RSI/ZoneRI.hpp"
#include "RSI/hungarian/hungarian.h"
#include "bettergraph/SimpleGraph.hpp"
#include "eigen3/Eigen/Core"
#include <eigen3/Eigen/Eigenvalues>
#include <opencv2/opencv.hpp>

namespace AASS {
	namespace graphmatch {


		class MatchLaplacian;


		class Region {
		protected:

			std::vector< std::vector< cv::Point > > _contour;
//			std::deque <cv::Point2i> _zone;
//			cv::Moments moment;
			cv::Point2f _center;



			double _uniqueness = -1;

			double _heat = -1;
			double _time = -1;

			double _heat_anchors = -1;

//			double _eigenvalue;
//			Eigen::VectorXd _eigenvector;

		public:

			AASS::RSI::ZoneRI zone;


			EIGEN_MAKE_ALIGNED_OPERATOR_NEW;
			int index = -1;
			Region(){}

			Region(const Region& r){
				_contour = r.getContour();
				_center = r.getCenter();
				_uniqueness = r.getUniqueness();
				_heat = r.getHeat();
				_time = r.getTime();
				_heat_anchors = r.getHeatAnchors();
				zone = r.zone;
			}

			void print() const {
				std::cout << "Node " << index << " heat " << _heat << " heat anchors " << _heat_anchors << " time " << _time << std::endl;
			}

			void setCenter(const cv::Point2f& center){_center = center;}
			void setContour(const std::vector< std::vector< cv::Point > >& contour){_contour = contour;}

			const cv::Point2f& getCenter() const {return _center;}
			const std::vector< std::vector< cv::Point > >& getContour() const {return _contour;}

			double getUniqueness() const {return _uniqueness;}
			void setUniqueness(double se){_uniqueness = se;}

			double getHeat() const {return _heat;}
			double getHeatAnchors() const {return _heat_anchors;}
			double getTime() const {return _time;}

			double heatKernel( const Eigen::VectorXd& eigenvalues, const Eigen::MatrixXd& eigenvectors, double time) {
				double score = 0;
				for (int i = 0; i < eigenvalues.size(); ++i){
					double eigenvalue = eigenvalues[i];
					double eigenvectorvalue = eigenvectors.col(i)(index);
					score = score + std::exp(-time * eigenvalue) * (eigenvectorvalue * eigenvectorvalue);
				}

				_heat = score;
				_time = time;
				return score;
			}

			double heatKernelAnchor( const Eigen::VectorXd& eigenvalues, const Eigen::MatrixXd& eigenvectors, double time, double index_anchor) {
				double score = 0;
				for (int i = 0; i < eigenvalues.size(); ++i){
					double eigenvalue = eigenvalues[i];
					double eigenvectorvalue = eigenvectors.col(i)(index);
					double eigenvectorvalue_anchor = eigenvectors.col(i)(index_anchor);
					score = score + std::exp(-time * eigenvalue) * (eigenvectorvalue * eigenvectorvalue_anchor);
				}
				return score;
			}

			double heatKernelAnchors( const Eigen::VectorXd& eigenvalues, const Eigen::MatrixXd& eigenvectors, double time, std::deque<int> indexes_anchor) {
				double score_anchors = 0;
				for(auto index : indexes_anchor) {
					score_anchors = score_anchors + heatKernelAnchor(eigenvalues, eigenvectors, time, index);
				}
				_heat_anchors = score_anchors;
				heatKernel(eigenvalues, eigenvectors, time);
				return score_anchors;
			}

			double compare(const Region& region) const {
				assert(region.getTime() == _time);
				assert(_heat_anchors != -1);
				std::cout << "Heats : " << region.getHeatAnchors() << " - " << _heat_anchors << std::endl;
//				return std::abs( region.getHeatAnchors() - _heat_anchors );
				return std::abs( region.getHeat() - _heat );
			}

//			void setEigen(double value, const Eigen::VectorXd& vector){
//				_eigenvalue = value;
//				_eigenvector = vector;
//			}

//			double getEigenValue(){return _eigenvalue;}
//			const Eigen::VectorXd& getEigenVector() const {return _eigenvector;}





		};


		inline std::ostream& operator<<(std::ostream& in, const Region &p){

			in << p.getCenter();
			return in;

		}

		inline bool operator==(const Region& in, const Region &p){

			return in.getHeatAnchors() == p.getHeatAnchors();

		}







		class EdgeType {
		public:
			EdgeType(){};
		};


		class GraphLaplacian : public bettergraph::SimpleGraph<Region, EdgeType> {

		public:
			EIGEN_MAKE_ALIGNED_OPERATOR_NEW;

			typedef typename bettergraph::SimpleGraph<Region, EdgeType>::GraphType GraphLaplacianType;
			typedef typename bettergraph::SimpleGraph<Region, EdgeType>::Vertex VertexLaplacian;
			typedef typename bettergraph::SimpleGraph<Region, EdgeType>::Edge EdgeLaplacian;
			typedef typename bettergraph::SimpleGraph<Region, EdgeType>::VertexIterator VertexIteratorLaplacian;
			typedef typename bettergraph::SimpleGraph<Region, EdgeType>::EdgeIterator EdgeIteratorLaplacian;

		protected:

			Eigen::VectorXd _eigenvalues;
			Eigen::MatrixXd _eigenvectors;
			std::deque<VertexLaplacian> _anchors;

		public:


			GraphLaplacian() {}

			void print() const {

				for(auto anchor : _anchors){
					std::cout << "Anchor index " << (*this)[anchor].index << " ";
				}
				std::cout << std::endl;


				std::pair<AASS::graphmatch::GraphLaplacian::VertexIteratorLaplacian, AASS::graphmatch::GraphLaplacian::VertexIteratorLaplacian> vp;
				//vertices access all the vertix
				for (vp = boost::vertices((*this)); vp.first != vp.second; ++vp.first) {
					auto v = *vp.first;
					(*this)[v].print();
				}
			}

			void addAnchor(const VertexLaplacian& anch){_anchors.push_back(anch);}

			/**
			 *
			 * @return the adjancy matrix with the sources as the rows and the target as columns
			 */
			Eigen::MatrixXd getAdjancyMatrix();
			Eigen::MatrixXd getWeightedGeneralizedLaplacian();

			std::tuple<Eigen::VectorXd, Eigen::MatrixXd> eigenLaplacian();

			void laplacianFamilySignatureGeneration(){};

			double getHeatKernelValueNode(const VertexLaplacian& vertex, double time){
				return (*this)[vertex].heatKernel(_eigenvalues, _eigenvectors, time);
			}

			void propagateHeatKernel(double time){

				std::deque<int> index_anchors;
				for(auto anchor : _anchors){
					index_anchors.push_back( (*this)[anchor].index );
				}

				std::pair<VertexIteratorLaplacian, VertexIteratorLaplacian> vp;
				for (vp = boost::vertices(*this); vp.first != vp.second; ++vp.first) {
					VertexLaplacian vertex_in = *vp.first;
					(*this)[vertex_in].heatKernelAnchors(_eigenvalues, _eigenvectors, time, index_anchors);
				}
			}

			std::vector<AASS::graphmatch::MatchLaplacian> compare(const GraphLaplacian& gl) const ;

			std::vector< AASS::graphmatch::MatchLaplacian > hungarian_matching(const AASS::graphmatch::GraphLaplacian& laplacian_model);


			void drawSpecial(cv::Mat& m, const VertexLaplacian& v, const cv::Scalar& color ) const
			{
				cv::circle(m, (*this)[v].getCenter(), 15, color, -1);
			}

			void drawSpecial(cv::Mat& m) const
			{

				cv::Scalar color_link;
				if(m.channels() == 1){
					color_link = 190;
				}
				else if(m.channels() == 3){
					color_link[0] = 0;
					color_link[1] = 0;
					color_link[2] = 255;
				}
				cv::RNG rrng(12345);


				//first is beginning, second is "past the end"
				std::pair<VertexIteratorLaplacian, VertexIteratorLaplacian> vp;
				//vertices access all the vertix
				for (vp = boost::vertices((*this)); vp.first != vp.second; ++vp.first) {

					cv::Scalar color_all_linked;

					if(m.channels() == 1){
						color_all_linked = rrng.uniform(50, 255);
					}
					else if(m.channels() == 3){
						color_all_linked[1] = rrng.uniform(50, 255);
						color_all_linked[2] = rrng.uniform(50, 255);
						color_all_linked[3] = rrng.uniform(50, 255);
					}

					VertexLaplacian v = *vp.first;
					drawSpecial(m, v, color_all_linked);

					EdgeIteratorLaplacian out_i, out_end;
					EdgeLaplacian e;

					for (boost::tie(out_i, out_end) = boost::out_edges(v, (*this));
					     out_i != out_end; ++out_i) {
						e = *out_i;
						VertexLaplacian src = boost::source(e, (*this)), targ = boost::target(e, (*this));
						cv::line(m, (*this)[src].getCenter(), (*this)[targ].getCenter(), color_link, 5);

					}

				}
			}





		};

	}
}

#endif