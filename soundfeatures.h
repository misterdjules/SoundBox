#ifndef SOUNDFEATURES_H_
#define SOUNDFEATURES_H_

class Peak
{
private:
    double m_PeakSampleIndex;
    double m_AttackSampleIndex;    

public:
    Peak(double peakSampleIndex, double attackSampleIndex) 
        : m_PeakSampleIndex(peakSampleIndex), m_AttackSampleIndex(attackSampleIndex)
    {}    
    
	void OffsetBy(unsigned int offset)
	{
		m_PeakSampleIndex	+= offset;
		m_AttackSampleIndex += offset;
	}

	struct OffsetByFunctor
	{
		unsigned int m_Offset;

		OffsetByFunctor(unsigned int offset) : m_Offset(offset) {}
		
		void operator()(Peak& peak)
		{
			peak.OffsetBy(m_Offset);
		}
	};

	void SetPeakSampleIndex(double peakSampleIndex) { m_PeakSampleIndex = peakSampleIndex; }
	void SetAttackSampleIndex(double attackSampleIndex) { m_AttackSampleIndex = attackSampleIndex; }

    double GetPeakSampleIndex() const { return m_PeakSampleIndex; }
    double GetAttackSampleIndex() const { return m_AttackSampleIndex; }
};

#endif // SOUNDFEATURES_H_