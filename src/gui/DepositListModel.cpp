// Copyright (c) 2011-2017 The Cryptonote developers
// Copyright (c) 2014-2017 XDN developers
// Copyright (c) 2018 The Circle Foundation
//
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "DepositListModel.h"
#include "DepositModel.h"
#include "SortedDepositModel.h"
#include "qdatetime.h"

namespace WalletGui {

DepositListModel::DepositListModel() : QSortFilterProxyModel() {
  setSourceModel(&SortedDepositModel::instance());
}

DepositListModel::~DepositListModel() {
}

bool DepositListModel::filterAcceptsColumn(int _sourceColumn, const QModelIndex& _sourceParent) const {
  return _sourceColumn == DepositModel::COLUMN_STATE || _sourceColumn == DepositModel::COLUMN_AMOUNT || _sourceColumn == DepositModel::COLUMN_TERM_RATE || _sourceColumn == DepositModel::COLUMN_INTEREST || _sourceColumn == DepositModel::COLUMN_UNLOCK_TIME;
}

bool DepositListModel::lessThan(const QModelIndex &left, const QModelIndex &right) const {
    QVariant leftData = sourceModel()->data(left);
    QVariant rightData = sourceModel()->data(right);
	
    if (leftData.type() == QVariant::String){
        {
            QDateTime leftVal = leftData.toDateTime();
            if (leftVal.isValid()){
                QDateTime rightVal = rightData.toDateTime();
                if (rightVal.isValid()) return leftVal < rightVal;
            }
        }
        {
            bool ok;
            double leftVal = leftData.toDouble(&ok);
            if (ok){
                double rightVal = rightData.toDouble(&ok);
                if (ok) return leftVal < rightVal;
            }
        }

        return leftData.toString() < rightData.toString();
    }
    else if (leftData.type() == QVariant::Int){
        return leftData.toInt() < rightData.toInt();
    }
    else if (leftData.type() == QVariant::LongLong){
        return leftData.toLongLong() < rightData.toLongLong();
    }
	else if (leftData.type() == QVariant::ULongLong){
        return leftData.toULongLong() < rightData.toULongLong();
    }
    else if (leftData.type() == QVariant::Double){
        return leftData.toDouble() < rightData.toDouble();
    } 
    else if (leftData.type() == QVariant::DateTime){
        return leftData.toDateTime() < rightData.toDateTime();
    }
    else {
		return leftData.toString() < rightData.toString();
    }
}

}
