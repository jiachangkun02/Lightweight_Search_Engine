#pragma once
#include <string>
#include <unordered_map>
#include <vector>
// boost���л���ص�ͷ�ļ������ڳ־û���������
#include <boost/serialization/access.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/unordered_map.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>


// ��������������ݽṹ�����ռ�
namespace jia
{
    // �����ͱ�ʾ�ĵ�ID�����������Ͳ���
    using DocId = int;

    // ����������ṹ�� - ��¼����term��ĳ���ĵ��еĳ������
    struct PostingEntry
    {
        DocId doc_id_;              // �ĵ�ID
        int frequency;              // ͳ�ƴ����ļ��г��ֵĴ���
        std::vector<int> position;  // �����ĵ��е��±�λ�ã�����λ����Ϣ���ں���֧�ֶ����ѯ


    private:
        // boost���л�֧�� - ���ڱ���ͼ�������
        friend class boost::serialization::access;

        template<class archive>
        void serialize(archive & ar, const unsigned int version)
        {
            ar & doc_id_;
            ar & frequency;
            ar & position;
        }
        
        
    };

    // �ĵ�Ԫ��Ϣ�ṹ�� - �����ĵ��Ļ�����Ϣ
    struct DocInF
    {
        std::string doc_path;    // �ĵ�·��
        int length;              // �����ĵ����ܴ�����ΪʲôҪ������أ�����ı���֪ʶ�Ҵ�����ϸ�Ľ��ܵ�

    private:
        // boost���л�֧�� - ���ڱ���ͼ����ĵ���Ϣ
        friend class boost::serialization::access;

        template<class archive>
        void serialize(archive & ar, const unsigned int version)
        {
            ar& length;
            ar& doc_path;
        }
        
    };

    // ���������ĵ��ṹ�� - ���ڷ����������������
    struct ScoredDoc
    {
        DocId doc_id;       // �ĵ�ID
        double score;       // �ĵ�������Ե÷�

        // ����>�����ʵ�ֻ��ڷ��������򣨽���
        bool operator>(const ScoredDoc& other) const
        {
            return score > other.score;
        }
        
    };
    
}