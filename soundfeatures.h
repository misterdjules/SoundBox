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
    
    double GetPeakSampleIndex() const { return m_PeakSampleIndex; }
    double GetAttackSampleIndex() const { return m_AttackSampleIndex; }
};

#endif // SOUNDFEATURES_H_