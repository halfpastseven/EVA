#ifndef FITTINGWINDOW_H
#define FITTINGWINDOW_H

#include "circuitmodel.h"
#include "circuitdiagram.h"
#include "dataseriestable.h"
#include "impedance.h"
#include "parametertablemodel.h"
#include "plotdualgraph.h"

#include <QButtonGroup>
#include <QDialog>
#include <QList>
#include <QRadioButton>
#include <QTableView>


class FittingWindow : public QDialog
{
    Q_OBJECT
public:
    FittingWindow(QWidget* parent, 
                  DataSeriesTable* SeriesTable, 
                  QList<impedance>* dataList);
    
public slots:
    
    void dataSourceChanged();
    void fittingModeSelected(int buttonId);
    void expSeriesAvailable (bool Enabled); // controls whether fitting mode is enabled or not

private:
    /// Data
    QList <impedance>* DataList; // Original list of all the data
    
    // The fitted data is a copy of an imp series from the 
    // main data source. It needs to be re-copied every time
    // the fitting window opens.
    QList <impedance> fittedData;
    
    DataSeriesTable* seriesTable;
    CircuitModel circuitModel;
    ParameterTableModel* parameterTableModel;
    
    QVector <double> defaultFreq;

    /// Window component    
    QButtonGroup*  fittingModeButtons; 
    QRadioButton*  selectSimButton;
    QRadioButton*  selectFitButton;
    QTableView*    seriesListView;
    QTableView*    parameterTableView;
    QLineEdit*     circuitExpressionEdit;
    CircuitDiagram* circuitDiagram;
    PlotDualGraph* FittingGraph;

    const int DEFAULT_DUAL_GRAPH_SIZE = 420;
    const int DEFAULT_LEFT_WIDTH = 320;
    
    /// Sim and fit
    bool simulate(impedance &result, const impedance *freqSource);
    bool setDefaultFreq(double maxF, double minF, int ptPerDec);
    
    
private slots:
    void dataSelectionChanged(const QVector<impedance *>& newData);
    void fittingDataSelected(QModelIndex index);
    void circuitExpressionAccepted();
    void startSimFitButtonPushed();
};

#endif // FITTINGWINDOW_H
