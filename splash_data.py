import numpy as np
import pandas as pd
import random
import sys
import math


class Splashes():
    def __init__(self, argv):
        """
        Take CSV DataFrames for HSC and SP data and the number of quants of energy.
        Index splashes from 0 and not from 1.
        """
        self.high_speed_camera_df = pd.read_csv(argv[0])
        self.sticky_paper_df = pd.read_csv(argv[1])
        self.number_of_quants_of_energy = int(argv[2])
        self.__shift_splashes_numbers()

    def __shift_splashes_numbers(self):
        """
        Index splashes from 0 and not from 1.
        """
        self.high_speed_camera_df['no'] -= 1
        self.sticky_paper_df['no'] -= 1

    def __velocity_outliers_of_hsc(self):
        """
        Return (indices of) outliers in DataFrame based on interquartile range (IQR).
        https://careerfoundry.com/en/blog/data-analytics/how-to-find-outliers/
        """
        q1 = self.high_speed_camera_df['v'].quantile(0.20)
        q3 = self.high_speed_camera_df['v'].quantile(0.80)
        iqr = q3 - q1
        
        outliers = self.high_speed_camera_df['v'][((self.high_speed_camera_df['v'] < (q1 - 1.5 * iqr))
                                                 | (self.high_speed_camera_df['v'] > (q3 + 1.5 * iqr)))]

        return outliers


    def drop_velocity_outliers_of_hsc(self):
        self.high_speed_camera_df = self.high_speed_camera_df.drop(self.__velocity_outliers_of_hsc().index.tolist())


    def mean_std_sp_to_hsc_cardinality(self, sample_count):
        """
        Return mean and std of the random quotient of cardinalities of SP and HSC splashes.

        Since there are only 16 splashes recorded by HSC we take a random sample of 16 splashes
        recorder by SP and divide the sum of beads in these 16 splashes by the sum of beads in
        all HSC splashes.
        """
        
        high_speed_camera_sum = sum([len(self.high_speed_camera_df[self.high_speed_camera_df['no'] == i]) for i in range(16)])
        sticky_paper_cardinalities = [len(self.sticky_paper_df[self.sticky_paper_df['no'] == i]) for i in range(48)]
        sp_hsc_quotient = [sum(random.sample(sticky_paper_cardinalities, 16)) / high_speed_camera_sum for i in range(sample_count)]

        return np.mean(sp_hsc_quotient), np.std(sp_hsc_quotient)


class EnergyStats():
    def __init__(self, splashes: Splashes):
        self.sum = [splashes.high_speed_camera_df[splashes.high_speed_camera_df['no'] == i]['e'].sum() for i in range(16)]
        self.mean = np.mean(self.sum)
        self.std = np.std(self.sum)
        self.min = np.min(self.sum)
        self.max = np.max(self.sum)
        self.min_particle = np.min(splashes.high_speed_camera_df['e'])
        self.max_particle = np.max(splashes.high_speed_camera_df['e'])
        self.scaling_mean, self.scaling_std = splashes.mean_std_sp_to_hsc_cardinality(1000)
        self.scaled_mean = self.mean * self.scaling_mean
        self.scaled_std = self.std * self.scaling_mean
        self.scaled_min = self.min * self.scaling_mean
        self.scaled_max = self.max * self.scaling_mean
        self.quant_of_energy = self.scaled_mean / number_of_quants_of_energy
        self.quantized_mean = self.scaled_mean / self.quant_of_energy
        self.quantized_std = self.scaled_std / self.quant_of_energy
        self.quantized_min = self.scaled_min / self.quant_of_energy
        self.quantized_max = self.scaled_max / self.quant_of_energy
        self.min_of_one_part = math.floor(self.min_particle / self.quant_of_energy)
        self.max_of_one_part = math.ceil(self.max_particle / self.quant_of_energy)
        self.min_number_of_parts = math.ceil(self.quantized_min / self.max_of_one_part)
        self.max_number_of_parts = math.floor(self.quantized_max  / self.min_of_one_part)

    def print_stats(self):
        print(f"Energy mean = {self.mean}, std = {self.std}")
        print(f"Energy min = {self.min}, max = {self.max}")
        print(f"Particle energy min = {self.min_particle}, max = {self.max_particle}")
        print(f"Scaling mean = {self.scaling_mean}, std = {self.scaling_std}, coeff of var = {self.scaling_std/self.scaling_mean}")
        print(f"Scaled energy mean = {self.scaled_mean}, std = {self.scaled_std}")
        print(f"Scaled energy min = {self.scaled_min}, std = {self.scaled_max}")
        print(f"Quant energy mean = {self.quantized_mean}, std = {self.quantized_std}")
        print(f"Quant energy min = {self.quantized_min}, max = {self.quantized_max}")
        print(f"kmin = {self.min_of_one_part}, kmax = {self.max_of_one_part}")
        print(f"parts min = {self.min_number_of_parts}, max = {self.max_number_of_parts}")


hsc_filename = sys.argv[1]
sp_filename = sys.argv[2]
number_of_quants_of_energy = int(sys.argv[3])

splashes = Splashes([hsc_filename, sp_filename, number_of_quants_of_energy])

# indices of sticky paper registered by hsc
# indices_of_hsc_splashes_in_sp = [1, 2, 3, 4, 5, 7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 18]

splashes.drop_velocity_outliers_of_hsc()

energy_stats = EnergyStats(splashes)

print(f"{splashes.number_of_quants_of_energy} {energy_stats.quantized_std:.4f} {energy_stats.quantized_min:.4f} {energy_stats.quantized_max:.4f} {energy_stats.min_of_one_part} {energy_stats.max_of_one_part}")
